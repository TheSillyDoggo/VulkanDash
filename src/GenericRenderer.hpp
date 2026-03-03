#pragma once

#include <vector>

class GenericRenderer
{
    protected:

    public:
        virtual bool init();
        virtual bool begin();
        virtual void present();
};