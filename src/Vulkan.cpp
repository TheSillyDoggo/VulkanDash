#include <Geode/Geode.hpp>
#include <Geode/modify/CCDirector.hpp>
#include <Geode/modify/CCEGLView.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <windows.h>
#include "VulkanRenderer.hpp"

using namespace geode::prelude;

GenericRenderer* getRenderer()
{
    static GenericRenderer* instance = nullptr;

    if (!instance)
        instance = new VulkanRenderer();

    return instance;
}

class $modify (CCEGLView)
{
    virtual void swapBuffers()
    {
        getRenderer()->present();
    }
};

class $modify (CCDirector)
{
    void setOpenGLView(CCEGLView *pobOpenGLView)
    {
        CCDirector::setOpenGLView(pobOpenGLView);

        static bool init = false;

        if (!init)
        {
            getRenderer()->init();
            init = true;
        }
    }

    void drawScene(void)
    {
        if (!getRenderer()->begin())
            return;

        CCDirector::drawScene();
    }
};