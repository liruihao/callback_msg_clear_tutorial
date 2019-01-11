#include "ros/ros.h"
#include <ros/callback_queue.h>
#include <ros/subscription_queue.h>
#include <ros/spinner.h>
#include "std_msgs/String.h"

boost::shared_ptr<ros::AsyncSpinner> async_spinner;

class MyQueue : public ros::CallbackQueue{
  public:
    void clearQueue(){
      ROS_WARN("ClearQueue:before size=%d", callbacks_.size());
      while(!callbacks_.empty()){
        uint64_t removalID = -1;
        {
          boost::mutex::scoped_lock lock(mutex_);
          removalID = callbacks_.begin()->removal_id;
          boost::shared_ptr<ros::SubscriptionQueue> qPtr = boost::dynamic_pointer_cast<ros::SubscriptionQueue>(callbacks_.begin()->callback);
          qPtr->clear();
        }
        removeByID(removalID);
        ROS_WARN("ClearQueue:after size=%d", callbacks_.size());
      }
    }
};


/**
 * The first callback function
 */
void chatterCallback_1(const std_msgs::String::ConstPtr& msg)
{
  ROS_INFO("Subscriber<1> heard: [%s]", msg->data.c_str());
}

/**
 * The second callback function
 */
void chatterCallback_2(const std_msgs::String::ConstPtr& msg)
{
  ROS_INFO("Subscriber<2> heard: [%s]", msg->data.c_str());
}

/**
 * The third callback function
 */
void chatterCallback_3(const std_msgs::String::ConstPtr& msg)
{
  ROS_INFO("Subscriber<3> heard: [%s]", msg->data.c_str());
}


int main(int argc, char **argv)
{
  /**
   * The ros::init() function
   */
  ros::init(argc, argv, "listener");
  ros::NodeHandle n;

  ros::Rate loop_rate(10);

  ros::CallbackQueue cq_1;
  //n.setCallbackQueue(&cq_1);

  //ros::CallbackQueue cq_2;
  MyQueue cq_2;
  //n.setCallbackQueue(&cq_2);

  /**
   * For sub_1 with chatterCallback_1,
   * nothing special for internal ROS queue  
   */
  ros::Subscriber sub_1 = n.subscribe("chatter", 100, chatterCallback_1);

  /**
   * For sub_2 with chatterCallback_2,
   * create options for subscriber and pass pointer to our custom queue
   */
  ros::SubscribeOptions ops =
    ros::SubscribeOptions::create<std_msgs::String>(
      "chatter",        // topic name
      100,              // queue length
      chatterCallback_2, // callback function
      ros::VoidPtr(),    // tracked object, we don't need one thus NULL
      &cq_2              // pointer to callback queue object
    );
  ros::Subscriber sub_2 = n.subscribe(ops);

  ros::SubscribeOptions ops1 =
    ros::SubscribeOptions::create<std_msgs::String>(
      "chatter",        // topic name
      100,              // queue length
      chatterCallback_3, // callback function
      ros::VoidPtr(),    // tracked object, we don't need one thus NULL
      &cq_2              // pointer to callback queue object
    );
  ros::Subscriber sub_3 = n.subscribe(ops1);
  /**
   *  spawn async spinner with 1 thread, running on our custom queue: cq_2
   *  ros::AsyncSpinner async_spinner(1, &cq_2);
   */
  async_spinner.reset(new ros::AsyncSpinner(1, &cq_2));
  /**
   *  start the  async_spinner
   */
  async_spinner->start();

  int clear_timer=0;
  bool clear_flag=false;
  while (ros::ok())
  {
    if (clear_timer>=100 && clear_timer<=200)
    {
      if (clear_flag==false)
      {
        async_spinner->stop();
        ROS_INFO("Spinner disabled!!!!!!");
        clear_flag=true;
      }
    }
    else
    {
      //std::cout<<"cq2--callbackqueue size: "<<cq_2.size_queue()<<std::endl;
      if (clear_flag==true)
      {
        /** 
         * Clear old callbacks queue and subscription queue
         */ 
        //cq_2.ClearSubCalQueue();
        cq_2.clearQueue();
        /**
         * After we start the spinner again, the messages in the subsriber 
         * will be push into the callbackqueue
         */ 
        async_spinner->start();
        ROS_INFO("Spinner Start again!!!!!!");
        clear_flag=false;
      }
    }
    //cq_2.callAvailable();
    //std::cout<<"cq2--callbackqueue size: "<<cq_2.size_queue()<<std::endl;
    ros::spinOnce();
    loop_rate.sleep();
       
    clear_timer++;
    ROS_INFO("clear timer: [%i]", clear_timer);
    
  }

  /**
   *  release the spinner
   */
  async_spinner.reset();

  return 0;
}
