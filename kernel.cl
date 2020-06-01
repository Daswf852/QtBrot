void kernel mandelkernel(global unsigned char *buffer, double upperLeftRe, double upperLeftIm, double pixelMult, unsigned int width, unsigned int height, unsigned int maxIterations) {
    
    int threadID = get_global_id(0);
    int nThreads = get_global_size(0);
    
    for (unsigned int y = (unsigned int)threadID; y < height; y += nThreads) {
        for (unsigned int x = 0; x < width; x++) {
            double cRe = upperLeftRe + ((double)x * pixelMult);
            double cIm = upperLeftIm + ((double)y * pixelMult);
            double zRe = cRe;
            double zIm = cIm;
            
            unsigned int currentIteration = 0;
            for (;currentIteration < maxIterations && (zRe*zRe + zIm*zIm) <= 4.f; currentIteration++) {
                double zReOld = zRe;
                zRe = (zRe * zRe) - (zIm * zIm);
                zIm = zReOld * zIm * 2;
                zRe += cRe;
                zIm += cIm;
            }
            
            if ((zRe*zRe + zIm*zIm) >= 4.f)
                buffer[x + (threadID*width)] = 255 - ((float)currentIteration/maxIterations*255);
            else 
                buffer[x + (threadID*width)] = 0;
            
        }
    }
}
