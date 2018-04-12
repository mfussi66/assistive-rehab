/******************************************************************************
 *                                                                            *
 * Copyright (C) 2018 Fondazione Istituto Italiano di Tecnologia (IIT)        *
 * All Rights Reserved.                                                       *
 *                                                                            *
 ******************************************************************************/

/**
 * @file Processor.cpp
 * @authors: Valentina Vasco <valentina.vasco@iit.it>
 */

#include "Processor.h"

using namespace std;
using namespace yarp::math;
using namespace assistive_rehab;

const string Rom_Processor::motion_type = "ROM";

Processor* createProcessor(const string& motion_tag, const Metric* metric_)
{
    if(motion_tag.compare(Rom_Processor::motion_type) >= 0)
    {
        yInfo() << "Creating processor for" << Rom_Processor::motion_type << "\n";
        return new Rom_Processor(metric_);
    }
    else
        return 0;
}

/********************************************************/
Processor::Processor()
{

}

void Processor::setInitialConf(const SkeletonWaist &skeleton_init_, const map<string, pair<string, double> > &keypoints2conf_)
{
    skeleton_init.update_fromstd(skeleton_init_.get_unordered());
    keypoints2conf = keypoints2conf_;

//    skeleton_init.print();
}

bool Processor::isStatic(const KeyPoint& keypoint)
{
    if(keypoints2conf[keypoint.getTag()].first == "static")
        return true;
    else
        return false;
}

void Processor::update(const SkeletonWaist &curr_skeleton_)
{
    curr_skeleton.update(curr_skeleton_.get_unordered());
}

bool Processor::isDeviatingFromIntialPose()
{
    bool isDeviating = false;
    for(unsigned int i=0; i<curr_skeleton.getNumKeyPoints(); i++)
    {
        if(isStatic(*curr_skeleton[i]) && isDeviatingFromIntialPose(*curr_skeleton[i], *skeleton_init[i]))
        {
            isDeviating = true;
            yWarning() << curr_skeleton[i]->getTag() << "deviating from initial pose";
        }
    }

    return isDeviating;
}

bool Processor::isDeviatingFromIntialPose(const KeyPoint& keypoint, const KeyPoint& keypoint_init)
{
    Vector curr_kp(keypoint.getPoint()), curr_kp_init(keypoint_init.getPoint());
    Vector curr_pose, initial_pose;

    //if the current keypoint has child and parent
    if(keypoint.getNumChild() && keypoint.getNumParent())
    {
        Vector curr_kp_parent = keypoint.getParent(0)->getPoint();
        Vector curr_kp_child = keypoint.getChild(0)->getPoint();

        Vector curr_kp_parent_init = keypoint_init.getParent(0)->getPoint();
        Vector curr_kp_child_init = keypoint_init.getChild(0)->getPoint();

        curr_pose = curr_kp + curr_kp_parent + curr_kp_child;
        initial_pose = curr_kp_init + curr_kp_parent_init + curr_kp_child_init;
    }
    else if(keypoint.getNumParent()) //if the current keypoint has parent and not child
    {
        Vector curr_kp_parent = keypoint.getParent(0)->getPoint();
        Vector curr_kp_parent_init = keypoint_init.getParent(0)->getPoint();

        curr_pose = curr_kp + curr_kp_parent;
        initial_pose = curr_kp_init + curr_kp_parent_init;
    }
    else
    {
        curr_pose = curr_kp;
        initial_pose = curr_kp_init;
    }

    double deviation = norm(curr_pose-initial_pose);
    if(deviation > keypoints2conf[keypoint.getTag()].second)
        return true;
    else
        return false;
}

/********************************************************/
Rom_Processor::Rom_Processor()
{

}

Rom_Processor::Rom_Processor(const Metric *rom_)
{
    rom = (Rom*)rom_;
}

//void Rom_Processor::setInitialConf(const SkeletonWaist &skeleton_init_, const map<string, pair<string, double> > &keypoints2conf_)
//{
//    skeleton_init.update(skeleton_init_.get_unordered());
//    keypoints2conf = keypoints2conf_;
//}

double Rom_Processor::computeMetric()
{
    //get reference keypoint from skeleton
    string tag_joint = rom->getTagJoint();

//    cout << "compute metric for " << tag_joint.c_str() << endl;
    Vector kp_ref = curr_skeleton[tag_joint]->getPoint();

    Vector v1;
    double theta;
    if(curr_skeleton[tag_joint]->getNumChild())
    {
        Vector kp_child = curr_skeleton[tag_joint]->getChild(0)->getPoint();

        Vector plane_normal = rom->getPlane();
        v1 = kp_child-kp_ref;
        double dist = dot(v1, plane_normal);
        v1 = v1-dist*plane_normal;

        Vector ref_dir = rom->getRefDir();

        double v1_norm = norm(v1);
        double v2_norm = norm(ref_dir);
        double dot_p = dot(v1, ref_dir);

        //        double minval = rom->getMinVal() * (M_PI/180);
        //        double maxval = rom->getMaxVal() * (M_PI/180);
        //        double range = M_PI;
        //        double scale = (maxval-minval)/range;
        //        theta = scale*acos(dot_p/(v1_norm*v2_norm))+minval;
        theta = acos(dot_p/(v1_norm*v2_norm));

        return ( theta * (180/M_PI) );
    }
    else
    {
        yError() << "The keypoint does not have a child ";
        return 0.0;
    }
}
