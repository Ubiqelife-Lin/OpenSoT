#include <yarp/os/all.h>
#include "sot_VelKinCon_ctrl.h"

#define dT 0.025 //[s]

class sot_VelKinCon_module: public yarp::os::RFModule
{
protected:
    sot_VelKinCon_ctrl *thr;
    bool ctrl_started;
    yarp::os::BufferedPort<yarp::os::Bottle> idle_port;
public:
    bool configure(int argc, char* argv[])
    {
        thr = new sot_VelKinCon_ctrl(dT, argc, argv);

        ctrl_started = false;

        idle_port.open("/sot_VelKinCon/idle:i");

        if(ctrl_started)
        {
            if(!thr->start())
            {
                std::cout<<"Problems Starting module, closing!"<<std::endl;
                delete thr;
                return false;
            }
            std::cout<<"Starting Module"<<std::endl;
        }
        else
            std::cout<<"Ctrl started in STOP mode! To run it send a true to idle:i port!"<<std::endl;

        return true;
    }

    virtual bool close()
    {
        thr->stop();
        delete thr;
        idle_port.close();
        return true;
    }

    virtual double getPeriod(){return 1.0;}
    virtual bool updateModule()
    {
        std::string start = "start";
        std::string stop = "stop";

        yarp::os::Bottle* bot = idle_port.read();

        if(!bot==NULL)
        {
            if((start.compare(bot->get(0).asString()) == 0) && !ctrl_started)
            {
                ctrl_started = true;
                if(!thr->start())
                {
                    std::cout<<"Problems Starting module, closing!"<<std::endl;
                    delete thr;
                    return false;
                }
                std::cout<<"Starting Module"<<std::endl;
            }
            if((stop.compare(bot->get(0).asString()) == 0) && ctrl_started)
            {
                ctrl_started = false;
                thr->stop();
                std::cout<<"Stopping Module"<<std::endl;
            }
        }

        return true;
    }
};

int main(int argc, char* argv[])
{
    yarp::os::Network yarp;
    if(!yarp.checkNetwork()){
        std::cout<<"yarpserver not running, pls run yarpserver"<<std::endl;
        return 0;}

    sot_VelKinCon_module mod;
    mod.configure(argc, argv);

    return mod.runModule();
}
