void kernel mandelkernel(global unsigned char *buffer, double upperLeftRe, double upperLeftIm, double pixelMult, unsigned int width, unsigned int height, unsigned int maxIterations) {
    
    int threadID = get_global_id(0);
    int nThreads = get_global_size(0);
    
    float fpixelMult = (float)pixelMult;
    unsigned char *currentLine = &buffer[threadID*width];
    
    for (int y = threadID; y < height; y += nThreads) {
        for (int x = 0; x < width; x++) {
            double cRe = upperLeftRe + (fpixelMult * x);
            double cIm = upperLeftIm + (fpixelMult * y);
            double zRe = cRe;
            double zIm = cIm;
            
            unsigned int currentIteration = 0;
            for (;currentIteration < maxIterations && (zRe*zRe + zIm*zIm) <= 4.f; currentIteration++) {
                double zReOld = zRe;
                zRe = (zRe * zRe) - (zIm * zIm) + cRe;
                zIm = zIm * zReOld * 2 + cIm;
            }
            
            if ((zRe*zRe + zIm*zIm) >= 4.f)
                currentLine[x] = 255 - ((float)currentIteration/maxIterations*255);
            else 
                currentLine[x] = 0;
            
        }
    }
}
