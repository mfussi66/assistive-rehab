/******************************************************************************
 *                                                                            *
 * Copyright (C) 2018 Fondazione Istituto Italiano di Tecnologia (IIT)        *
 * All Rights Reserved.                                                       *
 *                                                                            *
 ******************************************************************************/

/**
 * @file collector.cpp
 * @authors: Valentina Vasco <valentina.vasco@iit.it> Alexandre Antunes <alexandre.gomespereira@iit.it>
 */

#include <iostream>
#include <vector>
#include <list>
#include <iomanip>
#include <mutex>
#include <fstream>

#include <yarp/os/RFModule.h>
#include <yarp/os/ResourceFinder.h>
#include <yarp/os/Network.h>
#include <yarp/os/Log.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/BufferedPort.h>

#include <json/json.h>
#include <json/value.h>

#include "src/eventCollector_IDL.h"

// #include "matio.h"

using namespace std;
using namespace yarp::os;

/****************************************************************/
class Collector : public RFModule, public eventCollector_IDL
{
    //params
    RpcServer rpcPort;
    BufferedPort<Bottle> googleSpeechPort;
    BufferedPort<Bottle> googleProcessPort;
    vector<string> speech_events;
    vector<string> speech_process_events;
    bool starting;
    mutex mtx;

public:

    /********************************************************/
    bool attach(RpcServer &source)
    {
        return this->yarp().attachAsServer(source);
    }

    /********************************************************/
    bool configure(ResourceFinder &rf)
    {
        string moduleName = rf.check("name", Value("eventCollector")).asString();
        setName(moduleName.c_str());
        
        googleSpeechPort.open(("/" + getName() + "/speech:i").c_str());
        googleProcessPort.open(("/" + getName() + "/speech-process:i").c_str()); 
        rpcPort.open(("/" + getName() + "/cmd").c_str());
        attach(rpcPort);

        starting=false;

        return true;
    }

    /********************************************************/
    bool interruptModule()
    {
        googleSpeechPort.interrupt();
        googleProcessPort.interrupt();
        rpcPort.interrupt();
        yInfo() << "Interrupted module";
        return true;
    }

    /********************************************************/
    bool close()
    {
        googleSpeechPort.close();
        googleProcessPort.close();
        rpcPort.close();
        yInfo() << "Closed ports";
        return true;
    }

    /********************************************************/
    double getPeriod()
    {
        return 0.1;
    }

    /********************************************************/
    bool updateModule()
    {
        lock_guard<mutex> lg(mtx);

        if (!starting)
        {
            return true;
        }

        Bottle *speech=googleSpeechPort.read(false);
        Bottle *speech_process=googleProcessPort.read(false);

        if (speech)
        {
            if (speech->size()>0)
            {
                string content=speech->get(0).asString();
                speech_events.push_back(content);
            }
        }

        if (speech_process)
        {
            if (speech_process->size()>0)
            {
                string content=speech_process->get(0).asString();
                speech_process_events.push_back(content);
            }
        }

        
        return true;
    }

    /****************************************************************/
    bool save_json()
    {
        // Construct Json Database
        Json::Value jsonRoot;
        jsonRoot["Trial"] = {}; 
        std::cout << jsonRoot["Trial"].asString() << std::endl;

        Json::Value jsonTrialInstance;
        jsonTrialInstance["Instance"] = {}; 

        // Read file and overwrite Json database
        std::string filename ("/projects/config.json");
        std::ifstream config_doc(filename.c_str(), std::ifstream::binary);
        config_doc >> jsonTrialInstance; // write file to jsonRoot
        std::cout << jsonTrialInstance["Number"].asString() << std::endl;


        // Json::Value vec(Json::arrayValue);
        // vec.append(Json::Value("error1"));
        // vec.append(Json::Value("error2"));
        
        Json::Value jsonErrorMessage;
        jsonErrorMessage["google-speech"]="e1";
        jsonErrorMessage["google-speech-process"]="e1process";
        
        jsonTrialInstance["Speech"]["error-messages"].append(jsonErrorMessage);

        jsonErrorMessage["google-speech"]="e2";
        jsonErrorMessage["google-speech-process"]="e2process";
        jsonTrialInstance["Speech"]["error-messages"].append(jsonErrorMessage);
        
        jsonRoot["Trial"].append(jsonTrialInstance);

        std::cout << jsonRoot << std::endl;

        return true;

        // {
        //     "Trial" : 
        //     [         
                    // {
                    //     "Number": 0,
                    //     "Speech" : 
                    //     {
                    //             "error-messages" : [ 
                    //                 { "google-speech": "e1",
                    //                 "google-speech-process": "e1process"
                    //                 },
                    //                 { "google-speech": "e2",
                    //                 "google-speech-process": "e2process"
                    //                 }
                    //             ]
                    //     },
                    //     "Navigation" : 
                    //     {
                    //             "error-messages" : [ "e1", "e2" ]
                    //     }
                    // }
        //     ]
        // }


    }

    /****************************************************************/
    bool save()
    {
        // //save to out file
        // const char *filename = "myfile.mat";
        // mat_t *matfp = NULL; //matfp contains pointer to MAT file or NULL on failure
        // matfp = Mat_CreateVer(filename, NULL, MAT_FT_MAT5); //or MAT_FT_MAT4 / MAT_FT_MAT73
        // //don't forget to close file with Mat_Close(matfp);


        // char* fieldname = "MyStringVariable";
        // char* mystring = "Text";
        // char* mystringnew[] = { mystring, mystring };
        // char foo[3][3] = {{'a','b','c'}, {'d','e','f'},{'g','h','i'}};
        // yDebug()<<__LINE__<<strlen(foo[1]);
        // size_t dim[2] = { 2, 4 };
        // matvar_t *variable = Mat_VarCreate(fieldname, MAT_C_CHAR, MAT_T_UTF8, 2, dim, &mystringnew, 0);
        // Mat_VarWrite(matfp, variable, MAT_COMPRESSION_NONE); //or MAT_COMPRESSION_ZLIB
        // Mat_VarFree(variable);

        // const int first = 3; //rows
        // string array1d[first]= { "ciaoo", "hello", "brabr" };

        // char* fieldname1d = "array1d";
        // size_t dim1d[2] = { first, 5 };
        // matvar_t *variable1d = Mat_VarCreate(fieldname1d, MAT_C_CHAR, MAT_T_UTF8, 2, dim1d, &array1d, 0); //rank 1
        // Mat_VarWrite(matfp, variable1d, MAT_COMPRESSION_NONE);
        // Mat_VarFree(variable1d);

        // // fill 1d array
        // for (int i = 0; i < first; i++)
        //     array1d[i] = (i + 1);

        // // write
        // char* fieldname1d = "speech_process";
        // size_t dim1d[2] = { speech_process_events.size(), 50 };
        // matvar_t *variable1d = Mat_VarCreate(fieldname1d, MAT_C_CHAR, MAT_T_UTF8, 2, dim1d, &speech_process_events, 0); //rank 1
        // Mat_VarWrite(matfp, variable1d, MAT_COMPRESSION_NONE);
        // Mat_VarFree(variable1d);

        return true;
    }

    /****************************************************************/
    bool start() override
    {
        lock_guard<mutex> lg(mtx);
        yInfo()<<"Starting collecting";
        save_json();
        starting=true;
        return true;
    }

    /****************************************************************/
    bool stop() override
    {
        lock_guard<mutex> lg(mtx);
         
        // save();

        speech_events.clear();
        speech_process_events.clear();
        starting=false;
        return true;
    }

};


/****************************************************************/
int main(int argc, char *argv[])
{
    Network yarp;
    if (!yarp.checkNetwork())
    {
        yError()<<"Unable to find Yarp server!";
        return EXIT_FAILURE;
    }

    ResourceFinder rf;
    rf.setDefaultContext("eventCollector");
    rf.setDefaultConfigFile("config.ini");
    rf.configure(argc,argv);

    Collector collector;
    return collector.runModule(rf);
}

