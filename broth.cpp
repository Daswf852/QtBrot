#include "broth.hpp"

Broth::Broth(unsigned int screenW, unsigned int screenH) {
    window = std::make_unique<sf::RenderWindow>(sf::VideoMode(screenW, screenH, sf::Style::None), "Render window");
    window->setFramerateLimit(30);
    window->setActive(false);

    image = std::make_unique<sf::Image>();
    image->create(screenW, screenH);

    StartWorkers();

    SFMLWorker = std::thread(&Broth::SFMLWorkerFunc, this);

    SetScreenCenter(std::complex<brotDataType>(0.f, 0.f));
    //NotifyWorkers();
}

Broth::~Broth() {
    quitting = true;

    //Wait for the mandelbrot workers
    StopWorkers();

    //Wait for the SFML worker
    if (SFMLWorker.joinable())
        SFMLWorker.join();
}
void Broth::HandleEvent(sf::Event &event) {
    std::complex<brotDataType> posDelta = std::complex<brotDataType> (0,0);
    std::complex<brotDataType> mousePosNum = std::complex<brotDataType> (0,0);
    switch (event.type) {
        case sf::Event::Closed:
            window->close();
            break;
        case sf::Event::KeyPressed:
            switch (event.key.code) {
                case sf::Keyboard::Up:
                case sf::Keyboard::W:
                    posDelta = PixelCoordinatesToComplexPlane(0, -movementDelta);
                    break;
                case sf::Keyboard::Left:
                case sf::Keyboard::A:
                    posDelta = PixelCoordinatesToComplexPlane(-movementDelta, 0);
                    break;
                case sf::Keyboard::Down:
                case sf::Keyboard::S:
                    posDelta = PixelCoordinatesToComplexPlane(0, movementDelta);
                    break;
                case sf::Keyboard::Right:
                case sf::Keyboard::D:
                    posDelta = PixelCoordinatesToComplexPlane(movementDelta, 0);
                    break;
                case sf::Keyboard::I:
                    Zoom();
                    break;
                case sf::Keyboard::O:
                    Zoom(false);
                    break;
                default:
                    break;
            }
            upperLeftCoordinates += posDelta;
            NotifyWorkers();
            break;
        case sf::Event::MouseButtonPressed:
            mousePosNum = ScreenCoordinatesToComplexPlane(event.mouseButton.x, event.mouseButton.y);
            if (event.mouseButton.button == sf::Mouse::Left) {
                Zoom();
                SetScreenCenter(mousePosNum);
            } else if (event.mouseButton.button == sf::Mouse::Right) {
                Zoom(false);
                SetScreenCenter(mousePosNum);
            } else if (event.mouseButton.button == sf::Mouse::Middle) {
                SetScreenCenter(mousePosNum);
            }

            NotifyWorkers();
            break;
        case sf::Event::MouseWheelScrolled:
            if(event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
                Zoom((event.mouseWheelScroll.delta > 0));
            }
            NotifyWorkers();
            break;
        default:
            break;
    }
}

void Broth::SFMLWorkerFunc() {
    std::cout<<"SFML worker starting"<<std::endl;
    if (!window) return;
    window->setActive();

    while (window->isOpen()) {
        sf::Event event;
        while (AreWorkersDone() && window->pollEvent(event))
            HandleEvent(event);

        sf::Texture texture;
        sf::Sprite sprite;
        texture.loadFromImage(*image);
        sprite.setTexture(texture);
        window->clear(sf::Color::White);
        window->draw(sprite);

        if (!AreWorkersDone()) {
            if (drawWhileWorking)
                window->display();
        } else {
            if (measuringWorktime) {
                std::cout<<"One full process is done"<<std::endl;
                measuringWorktime = false;
                passedSeconds = worktime.getElapsedTime().asSeconds();
                if (notifyCallback) notifyCallback();
            }
            window->display();
        }

        if (quitting) {
            window->close();
        }

        if (renderCallback) renderCallback();
    }
    std::cout<<"SFML worker stopping"<<std::endl;
}

void Broth::StartWorkers() {
    StopWorkers();

    if (brotMethod == OpenCL) {
        nThreads = 1;
        currentFrameIDs = std::vector<unsigned long long>(nThreads);
        std::fill(currentFrameIDs.begin(), currentFrameIDs.end(), 0);
        expectedFrameID = 0;
        mandelbrotWorkers = std::vector<std::thread>(nThreads);
        mandelbrotWorkers.at(0) = std::thread(&Broth::CLWorker, this);
    } else {
        currentFrameIDs = std::vector<unsigned long long>(nThreads);
        std::fill(currentFrameIDs.begin(), currentFrameIDs.end(), 0);
        expectedFrameID = 0;

        mandelbrotWorkers = std::vector<std::thread>(nThreads);

        for (size_t threadID = 0; threadID < mandelbrotWorkers.size(); threadID++) {
            switch (brotMethod) {
                default:
                case None:
                    mandelbrotWorkers.at(threadID) = std::thread(&Broth::NoneWorker, this, threadID);
                    break;
                case BasicMT:
                    mandelbrotWorkers.at(threadID) = std::thread(&Broth::BasicMTWorker, this, threadID);
                    break;
                case OptMT:
                    mandelbrotWorkers.at(threadID) = std::thread(&Broth::OptimisedMTWorker, this, threadID);
                    break;

            }
        }
    }
}

void Broth::NotifyWorkers() {
    if (!AreWorkersDone()) return;
    worktime.restart();
    measuringWorktime = true;
    std::cout<<"Notifying workers"<<std::endl;
    ++expectedFrameID;
    cv.notify_all();
    //if (notifyCallback) notifyCallback();
}

void Broth::StopWorkers() {
    stopWorkers = true;
    cv.notify_all();
    for (size_t i = 0; i < mandelbrotWorkers.size(); i++) {
        if (mandelbrotWorkers.at(i).joinable())
            mandelbrotWorkers.at(i).join();
    }
    stopWorkers = false;
}

void Broth::NoneWorker(size_t threadID) {
    std::cout<<"Worker "<<threadID<<" is starting"<<std::endl;
    while (!stopWorkers) {
        std::cout<<"Worker "<<threadID<<" is waiting"<<std::endl;
        std::unique_lock<std::mutex> lk(cv_m);
        cv.wait(lk);
        if (stopWorkers) break;
        lk.unlock();

        std::cout<<"Worker "<<threadID<<" is working"<<std::endl;
        ++currentFrameIDs[threadID];
    }
    std::cout<<"Worker "<<threadID<<" is stopping"<<std::endl;
}

void Broth::BasicMTWorker(size_t threadID) {
    std::cout<<"Worker "<<threadID<<" is starting"<<std::endl;

    size_t nThreads = mandelbrotWorkers.size();

    while (!stopWorkers) {
        std::cout<<"Worker "<<threadID<<" is waiting"<<std::endl;
        std::unique_lock<std::mutex> lk(cv_m);
        cv.wait(lk);
        if (stopWorkers) break;
        lk.unlock();

        std::cout<<"Worker "<<threadID<<" is working"<<std::endl;

        for (unsigned int y = threadID; y < image->getSize().y; y += nThreads) {
            for (unsigned int x = 0; x < image->getSize().x; x++) {
                std::complex<brotDataType> c = ScreenCoordinatesToComplexPlane(x, y);
                std::complex<brotDataType> z = c;

                unsigned int currentIteration;
                for (currentIteration = 0; currentIteration < maxIterations && DistFromOriginSq(z) < threshold*threshold; currentIteration++) {
                    z *= z;
                    z += c;
                }

                int color = 0;

                if (DistFromOrigin(z) >= threshold) {
                    color = 255 - ((float)currentIteration/(float)maxIterations*255);
                }

                image->setPixel(x, y, sf::Color(color, 0, 0));
            }
        }

        ++currentFrameIDs[threadID];
    }

    std::cout<<"Worker "<<threadID<<" is stopping"<<std::endl;
}

//mostly self contained
void Broth::OptimisedMTWorker(size_t threadID) {
    std::cout<<"Worker "<<threadID<<" is starting"<<std::endl;

    size_t nThreads = mandelbrotWorkers.size();

    while (!stopWorkers) {
        std::cout<<"Worker "<<threadID<<" is waiting"<<std::endl;
        std::unique_lock<std::mutex> lk(cv_m);
        cv.wait(lk);
        if (stopWorkers) break;
        lk.unlock();

        std::cout<<"Worker "<<threadID<<" is working"<<std::endl;

        brotDataType upperLeftRe = std::real(upperLeftCoordinates);
        brotDataType upperLeftIm = std::imag(upperLeftCoordinates);
        brotDataType pixelMult = (brotDataType)(1.f)/(brotDataType)(pixelDivider);

        for (unsigned int y = threadID; y < image->getSize().y; y += nThreads) {
            for (unsigned int x = 0; x < image->getSize().x; x++) {
                brotDataType cRe = upperLeftRe + (pixelMult * (brotDataType)x);
                brotDataType cIm = upperLeftIm + (pixelMult * (brotDataType)y);
                brotDataType zRe = cRe;
                brotDataType zIm = cIm;

                unsigned int currentIteration;
                for (currentIteration = 0; currentIteration < maxIterations && _QUICKDISTSQ(zRe, zIm) < threshold*threshold; currentIteration++) {
                    //z *= z;
                    //z += c;
                    brotDataType zReOld = zRe;
                    zRe = (zRe * zRe) - (zIm * zIm);
                    zIm = 2 * zReOld * zIm;
                    zRe += cRe;
                    zIm += cIm;
                }

                int color = 0;

                if (_QUICKDISTSQ(zRe, zIm) >= threshold*threshold) {
                    color = 255 - ((float)currentIteration/(float)maxIterations*255);
                }

                image->setPixel(x, y, sf::Color(color, 0, 0));
            }
        }

        ++currentFrameIDs[threadID];
    }

    std::cout<<"Worker "<<threadID<<" is stopping"<<std::endl;
}

//Just a sloppy OpenCL worker
//No guarantees whatsoever for anything, might burn your gpu for all i care
void Broth::CLWorker() {
    std::cout<<"CLWorker is starting"<<std::endl;

    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    assert(platforms.size() != 0); //asserting. fight me.
    cl::Platform platform = platforms.at(0);

    std::cout<<"CL platform: "<<platform.getInfo<CL_PLATFORM_NAME>()<<std::endl;

    std::vector<cl::Device> devices;
    platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
    assert(devices.size() != 0); // :^)
    cl::Device device = devices.at(0);

    std::cout<<"CL device: "<<device.getInfo<CL_DEVICE_NAME>()<<std::endl;

    cl::Context context({device});

    cl::Program::Sources progSources;

    std::ifstream ifs("/home/daswf852/Development/QtBrot/kernel.cl");
    assert (ifs.is_open());
    std::ostringstream oss;
    oss<<ifs.rdbuf();
    std::string kernelCode = oss.str();
    oss.str("");
    ifs.close();

    progSources.push_back({kernelCode.c_str(), kernelCode.size()});

    cl::Program program(context, progSources);
    if (program.build({device}, "") != CL_SUCCESS) {
        std::cout<<program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device)<<std::endl;
        assert(program.build({device}) == CL_SUCCESS);
    }
    cl::Kernel programKernel(program, "mandelkernel");

    unsigned int bufferSize = window->getSize().x * window->getSize().y;

    //C buffer
    uint8_t *CBuffer = new uint8_t[bufferSize];
    std::fill(CBuffer, CBuffer + bufferSize, 0);

    //CL buffer
    cl::Buffer outBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY, sizeof(uint8_t)*bufferSize);

    cl::CommandQueue queue(context, device);

    while (!stopWorkers) {
        std::cout<<"CLWorker is waiting"<<std::endl;
        std::unique_lock<std::mutex> lk(cv_m);
        cv.wait(lk);
        if (stopWorkers) break;
        lk.unlock();

        programKernel.setArg(0, outBuffer);
        programKernel.setArg(1, (double)std::real(upperLeftCoordinates));
        programKernel.setArg(2, (double)std::imag(upperLeftCoordinates));
        programKernel.setArg(3, (double)1.f/pixelDivider);
        programKernel.setArg(4, (unsigned int)window->getSize().x);
        programKernel.setArg(5, (unsigned int)window->getSize().y);
        programKernel.setArg(6, (unsigned int)this->maxIterations);

        queue.enqueueNDRangeKernel(programKernel, cl::NullRange, cl::NDRange(window->getSize().y));
        queue.enqueueReadBuffer(outBuffer, CL_FALSE, 0, sizeof(uint8_t)*bufferSize, CBuffer);

        queue.finish();

        unsigned int width = window->getSize().x;
        unsigned int height = window->getSize().y;
        for (unsigned int y = 0; y < height; y++) {
            for (unsigned int x = 0; x < width; x++) {
                image->setPixel(x, y, sf::Color(CBuffer[x+(y*width)], 0, 0));
            }
        }

        ++currentFrameIDs[0];
    }

    std::cout<<"CLWorker is stopping"<<std::endl;
}

inline Broth::brotDataType Broth::DistFromOrigin(brotDataType re, brotDataType im) {
    return std::sqrt(re*re + im*im);
}

inline Broth::brotDataType Broth::DistFromOrigin(std::complex<brotDataType> z) {
    return DistFromOrigin(std::real(z), std::imag(z));
}

inline Broth::brotDataType Broth::DistFromOriginSq(brotDataType re, brotDataType im) {
    return re*re + im*im;
}

inline Broth::brotDataType Broth::DistFromOriginSq(std::complex<brotDataType> z) {
    return DistFromOriginSq(std::real(z), std::imag(z));
}

inline std::complex<Broth::brotDataType> Broth::PixelCoordinatesToComplexPlane(int x, int y) {
    return std::complex<brotDataType>(x/pixelDivider, y/pixelDivider);
}

inline std::complex<Broth::brotDataType> Broth::ScreenCoordinatesToComplexPlane(int x, int y) {
    return std::complex<brotDataType>(std::real(upperLeftCoordinates) + (x/pixelDivider), std::imag(upperLeftCoordinates) + (y/pixelDivider));
}

void Broth::SetScreenCenter(std::complex<brotDataType> z) {
    std::complex<brotDataType> currentCenterNumber = ScreenCoordinatesToComplexPlane(window->getSize().x/2, window->getSize().y/2);
    std::complex<brotDataType> delta = z - currentCenterNumber;
    upperLeftCoordinates += delta;
}

void Broth::Zoom(bool in) {
    std::complex<brotDataType> oldCenter = std::complex<brotDataType>(0,0);
    oldCenter = ScreenCoordinatesToComplexPlane(window->getSize().x/2, window->getSize().y/2);
    if (in)
        pixelDivider += pixelDivider/(zoomDeltaDivider);
    else
        pixelDivider -= pixelDivider/(zoomDeltaDivider);
    SetScreenCenter(oldCenter);
}

///////////////////////////
/// Getters And Setters ///
///////////////////////////

void Broth::SetRenderCallback(std::function<void()> callback) {
    renderCallback = callback;
}

void Broth::SetNotifyCallback(std::function<void()> callback) {
    notifyCallback = callback;
}

void Broth::SetMaxIterations(unsigned int iterations) {
    if (!AreWorkersDone()) return;
    maxIterations = iterations;
    NotifyWorkers();
}

unsigned int Broth::GetMaxIterations() {
    return maxIterations;
}

void Broth::SetZoomRatio(double zoomRatio) {
    if (!AreWorkersDone()) return;
    this->zoomDeltaDivider = zoomRatio;
    NotifyWorkers();
}

double Broth::GetZoomRatio() {
    return zoomDeltaDivider;
}

void Broth::SetDrawWhileUpdating(bool b) {
    if (!AreWorkersDone()) return;
    drawWhileWorking = b;
    NotifyWorkers();
}

bool Broth::GetDrawWhileUpdating() {
    return drawWhileWorking;
}

void Broth::SetProcessMethod(EProcessMethod method) {
    if (!AreWorkersDone()) return;
    StopWorkers();
    brotMethod = method;
    StartWorkers();
}

Broth::EProcessMethod Broth::GetProcessMethod() {
    return brotMethod;
}

void Broth::SetThreads(size_t nThreads) {
    if (!AreWorkersDone()) return;
    StopWorkers();
    this->nThreads = nThreads;
    StartWorkers();
}

size_t Broth::GetThreads() {
    return nThreads;
}

std::complex<Broth::brotDataType> Broth::GetCenterCoordinates() {
    return ScreenCoordinatesToComplexPlane(window->getSize().x/2, window->getSize().y/2);
}

Broth::brotDataType Broth::GetPixelRatio() {
    return pixelDivider;
}

float Broth::GetPassedTime() {
    return passedSeconds;
}

bool Broth::AreWorkersDone() {
    for (auto ll : currentFrameIDs) {
        if (ll < expectedFrameID) return false;
    }
    for (auto it = currentFrameIDs.begin(); it != currentFrameIDs.end(); it++) *it = expectedFrameID;
    return true;
}

void Broth::RestartWorkers() {
    if (!AreWorkersDone()) return;
    StopWorkers();
    StartWorkers();
}

void Broth::SetOpenCLKernelPath(std::string path) {
    if (!AreWorkersDone()) return;
    kernelPath = path;
    RestartWorkers();
}
