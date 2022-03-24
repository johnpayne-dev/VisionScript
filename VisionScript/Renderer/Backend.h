#ifndef Backend_h
#define Backend_h

void InitializeGraphicsBackend(int32_t width, int32_t height);

void RecreateSwapchain(int32_t width, int32_t height);

void UpdateBackend(void);

void StartCompute(void);

void EndCompute(void);

void AquireNextSwapchainImage(void);

void PresentSwapchainImage(void);

#endif
