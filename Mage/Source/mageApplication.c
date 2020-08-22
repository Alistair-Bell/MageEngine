#include "mageAPI.h"

mageResult mageEngineInitialise()
{
    uint32_t result = glfwInit();
    if (result != GLFW_TRUE)
    {
        MAGE_LOG_CORE_INFORM("GLFW has failed to be created\n", NULL);
    }
    assert(result == GLFW_TRUE);
    
    return MAGE_RESULT_SUCCESS;
}
mageResult mageApplicationCreate(struct mageApplication *application, struct mageApplicationCreateInfo applicationInfo, struct mageRendererCreateInfo rendererInfo)
{
    application->Window             = MAGE_MEMORY_ALLOCATE(sizeof(struct mageWindow));
    application->Renderer           = MAGE_MEMORY_ALLOCATE(sizeof(struct mageRenderer));
    application->RendererCreateInfo = rendererInfo;

    application->Running = 1;
    mageResult returnCode = mageEngineInitialise();
    if (returnCode != MAGE_RESULT_SUCCESS) { return returnCode; }
    mageWindowCreate(application->Window, &applicationInfo);
    mageRendererCreate(application->Renderer, application->Window, &application->RendererCreateInfo);
    mageInputSetup(application);
    return MAGE_RESULT_SUCCESS;
}
void mageApplicationDestroy(struct mageApplication *application)
{
    mageWindowDestroy(application->Window);
    mageRendererDestroy(application->Renderer, &application->RendererCreateInfo);
    MAGE_MEMORY_FREE(application->Window);
    MAGE_MEMORY_FREE(application->Renderer);
    
    MAGE_LOG_CORE_INFORM("Application has been cleaned up\n", NULL);
}
