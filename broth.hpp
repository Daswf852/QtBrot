#ifndef BROTH_H
#define BROTH_H

#define _QUICKDISTSQ(x, y) x*x+y*y

#include <atomic>
#include <cassert>
#include <complex>
#include <condition_variable>
#include <fstream>
#include <functional>
#include <memory>
#include <iostream>
#include <thread>

#include <CL/cl.hpp>
#include <SFML/Graphics.hpp>

class Broth {
    public:
        enum EProcessMethod {
            None, BasicMT, OptMT, OpenCL
        };

        typedef double brotDataType;

        Broth(unsigned int screenW, unsigned int screenH);
        ~Broth();

        void SetRenderCallback(std::function<void()> callback);
        void SetNotifyCallback(std::function<void()> callback);

        void SetMaxIterations(unsigned int iterations);
        unsigned int GetMaxIterations();

        void SetZoomRatio(double zoomRatio);
        double GetZoomRatio();

        void SetDrawWhileUpdating(bool b);
        bool GetDrawWhileUpdating();

        void SetProcessMethod(EProcessMethod method);
        EProcessMethod GetProcessMethod();

        void SetThreads(size_t nThreads);
        size_t GetThreads();

        std::complex<brotDataType> GetCenterCoordinates();
        brotDataType GetPixelRatio();
        float GetPassedTime();

        bool AreWorkersDone();

        void RestartWorkers();

        void SetOpenCLKernelPath(std::string path);

    private:
        //Variables for the mandelbrot set itself
        //Complex plane
        std::complex<brotDataType> upperLeftCoordinates = std::complex<brotDataType>(0, 0);
        brotDataType pixelDivider = 100.f;

        //Iterations
        unsigned int maxIterations = 255;
        float threshold = 2.f;

        //Movement and zoom
        int movementDelta = 16; //movement in pixels
        brotDataType zoomDeltaDivider = 10.f;

        //SFML related stuff

        std::function<void()> renderCallback;
        std::function<void()> notifyCallback;

        bool quitting = false; //signal for all threads to stop

        std::unique_ptr<sf::RenderWindow> window;
        std::unique_ptr<sf::Image> image = nullptr;

        std::thread SFMLWorker;

        void HandleEvent(sf::Event &event);
        void SFMLWorkerFunc();

        bool drawWhileWorking = false;

        bool measuringWorktime = false;
        sf::Clock worktime;
        float passedSeconds = 0.f;

        //Threading related stuff

        EProcessMethod brotMethod = BasicMT;

        size_t nThreads = 8;

        std::vector<std::thread> mandelbrotWorkers;
        std::condition_variable cv;
        bool stopWorkers = false;
        std::mutex cv_m;

        unsigned long long expectedFrameID = 0;
        std::vector<unsigned long long> currentFrameIDs;

        std::string kernelPath = "./kernel.cl";

        //Will first try to stop any existing workers then it will start workers
        void StartWorkers();

        //Will trigger workers to generate stuff
        void NotifyWorkers();

        //Will stop the workers and wait for them to stop
        void StopWorkers();

        //Workers' functions

        void NoneWorker(size_t threadID);
        void BasicMTWorker(size_t threadID);
        void OptimisedMTWorker(size_t threadID);
        void CLWorker();

        //Helper functions
        inline brotDataType DistFromOrigin(brotDataType re, brotDataType im);
        inline brotDataType DistFromOrigin(std::complex<brotDataType> z);
        inline brotDataType DistFromOriginSq(brotDataType re, brotDataType im);
        inline brotDataType DistFromOriginSq(std::complex<brotDataType> z);

        inline std::complex<brotDataType> PixelCoordinatesToComplexPlane(int x, int y);
        inline std::complex<brotDataType> ScreenCoordinatesToComplexPlane(int x, int y);

        void SetScreenCenter(std::complex<brotDataType> z);
        void Zoom(bool in = true);

};

#endif // BROTH_H
