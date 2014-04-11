#include "fallrisk_gui.h"
#include "ui_fallrisk_gui.h"
#include "rviz/default_plugin/view_controllers/fixed_orientation_ortho_view_controller.h"
#include <iostream>
#include "rviz/view_manager.h"
#include "rviz/default_plugin/view_controllers/orbit_view_controller.h"
#include "rviz/default_plugin/tools/measure_tool.h"
#include "rviz/tool_manager.h"
#include "rviz/default_plugin/tools/point_tool.h"


FallRiskGUI::FallRiskGUI(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::FallRiskGUI),it_(nh_)
{
    ui->setupUi(this);
    ui->sliderLinearVel->setValue(75);
    ui->sliderAngularVel->setValue(75);
    //    ui->cbBedroomItem1->setStyleSheet("QCheckBox { background-color : red; color : black; };");
    //    ui->cbBedroomItem2->setStyleSheet("QCheckBox { background-color : yellow; color : black; };");
    //    ui->cbBedroomItem3->setStyleSheet("QCheckBox { background-color : green; color : black; };");
    ui->lbLightingItem1->setStyleSheet("QLabel { background-color : red; color : rgb(255, 255, 255); }");
    ui->lbLightingItem2->setStyleSheet("QLabel { background-color : green; color : rgb(255, 255, 255); }");
    ui->lbFloorsItem1->setStyleSheet("QLabel { background-color : green; color : rgb(255, 255, 255); }");
    ui->lbFloorsItem2->setStyleSheet("QLabel { background-color : green; color : rgb(255, 255, 255); }");
    ui->lbStairsItem1->setStyleSheet("QLabdel { background-color : green; color : rgb(255, 255, 255); }");
    ui->lbStairsItem2->setStyleSheet("QLabel { background-color : green; color : rgb(255, 255, 255); }");
    ui->lbStairsItem3->setStyleSheet("QLabel { background-color : red; color : rgb(255, 255, 255); }");
    ui->lbBedroomItem1->setStyleSheet("QLabel { background-color : green; color : rgb(255, 255, 255); }");
    ui->lbBedroomItem2->setStyleSheet("QLabel { background-color : red; color : rgb(255, 255, 255); }");
    ui->lbBedroomItem3->setStyleSheet("QLabel { background-color : green; color : rgb(255, 255, 255); }");
    ui->lbPetItem1->setStyleSheet("QLabel { background-color : yellow; color : rgb(255, 255, 255); }");

    initVariables();
    initDisplayWidgets();
    initTools();
    initActionsConnections();

    //just for testing, needs to be commented out
    //    cv::namedWindow("Image window");
}

FallRiskGUI::~FallRiskGUI()
{
    delete ui;
    delete mapManager_;
    delete mapRenderPanel_;
    delete manager_;
    delete renderPanel_;

    //    cv::destroyWindow("Image window");
}

void FallRiskGUI::initVariables()
{
    fixedFrame_ =  QString("/base_link");
    mapTopic_ = QString("/map");
    imageTopic_ = QString("/camera/rgb/image_raw"); ;
    pointCloudTopic_=QString("/camera/depth/points");
    octomapTopic_=QString( "/occupied_cells_vis_array" );
    baseSensorTopic_=QString("/mobile_base/sensors/core");
    velocityTopic_=QString("/mobile_base/commands/velocity");

    moveBaseCmdPub = nh_.advertise<geometry_msgs::Twist>(velocityTopic_.toStdString(),1);
    centerDistSub = nh_.subscribe("/distance/image_center_dist",1,&FallRiskGUI::distanceSubCallback,this);
    baseSensorStatus = nh_.subscribe(baseSensorTopic_.toStdString(),1,&FallRiskGUI::baseStatusCheck,this);
    liveVideoSub = it_.subscribe(imageTopic_.toStdString(),1,&FallRiskGUI::liveVideoCallback,this,image_transport::TransportHints("compressed"));

    setRobotVelocity();
}

void FallRiskGUI::initActionsConnections()
{
    connect(ui->btnUp, SIGNAL(clicked()), this, SLOT(moveBaseForward()));
    connect(ui->btnDown, SIGNAL(clicked()), this, SLOT(moveBaseBackward()));
    connect(ui->btnLeft, SIGNAL(clicked()), this, SLOT(moveBaseLeft()));
    connect(ui->btnRight, SIGNAL(clicked()), this, SLOT(moveBaseRight()));

//    connect(ui->btnMeasure, SIGNAL(clicked()), this, SLOT(getDistance()));
//    connect(ui->btnRvizInteract, SIGNAL(clicked()), this, SLOT(setCurrentTool()));
//    connect(ui->btnRvizMeasure, SIGNAL(clicked()), this, SLOT(setCurrentTool()));
//    connect(ui->btnRvizNavGoal, SIGNAL(clicked()), this, SLOT(setCurrentTool()));
//    connect(ui->btnRvizPoseEstimate, SIGNAL(clicked()), this, SLOT(setCurrentTool()));
//    connect(ui->btnRvizPublishPoint, SIGNAL(clicked()), this, SLOT(setCurrentTool()));
    connect(ui->btnGroupRvizTools,SIGNAL(buttonClicked(int)),this,SLOT(setCurrentTool(int)));

//    connect(ui->btnRvizInteract, SIGNAL(clicked()), this, SLOT(toolManager_->setCurrentTool(interactTool_);));

    connect(ui->sliderLinearVel, SIGNAL(valueChanged(int)),this,SLOT(setRobotVelocity()));
    connect(ui->sliderAngularVel, SIGNAL(valueChanged(int)),this,SLOT(setRobotVelocity()));
}

void FallRiskGUI::initDisplayWidgets()
{

    //Setup the UI elements for displaying 2D map
    mapRenderPanel_ = new rviz::RenderPanel();
    ui->map_layout->addWidget(mapRenderPanel_);
    mapManager_ = new rviz::VisualizationManager( mapRenderPanel_ );
    mapRenderPanel_->initialize( mapManager_->getSceneManager(), mapManager_);
    mapManager_->setFixedFrame(fixedFrame_);
    mapManager_->initialize();
    mapManager_->startUpdate();

    //Create and assign FixedOrientationOrthoViewController to the existing viewmanager of the visualization manager
    viewManager_ = mapManager_->getViewManager();
    viewManager_->setCurrentViewControllerType("rviz/TopDownOrtho");
    viewController_ = viewManager_->getCurrent();

    //Set parameters of the view controller to show map correctly
    viewController_->subProp("X")->setValue(0);
    viewController_->subProp("Y")->setValue(0);
    viewController_->subProp("Angle")->setValue(0);
    viewController_->subProp("Scale")->setValue(20);

    // Create a map display
    mapDisplay_ = mapManager_->createDisplay( "rviz/Map", "2D Map view", true );
    ROS_ASSERT( mapDisplay_ != NULL );

    mapDisplay_->subProp( "Topic" )->setValue( mapTopic_ );
    mapManager_->createDisplay( "rviz/RobotModel", "Turtlebot", true );

    // Initialize GUI elements for main panel
    renderPanel_ = new rviz::RenderPanel();
    ui->display3d_layout->addWidget(renderPanel_);

    manager_ = new rviz::VisualizationManager( renderPanel_ );
    renderPanel_->initialize( manager_->getSceneManager(), manager_ );

    //set the fixed frame before initializing Visualization Manager. pointcloud2 will not work with this
    manager_->setFixedFrame(fixedFrame_);
    manager_->initialize();
    manager_->startUpdate();


    // Create a main display to show pointcloud and octomap
    mainDisplay_ = manager_->createDisplay( "rviz/PointCloud2", "3D Pointcloud view", true );
    ROS_ASSERT( mainDisplay_ != NULL );

    mainDisplay_->subProp( "Topic" )->setValue( pointCloudTopic_ );
    mainDisplay_->subProp( "Selectable" )->setValue( "true" );
    mainDisplay_->subProp( "Style" )->setValue( "Boxes" );
    mainDisplay_->subProp("Alpha")->setValue(0.5);
    manager_->createDisplay( "rviz/Grid", "Grid", true );
    manager_->createDisplay( "rviz/RobotModel", "Turtlebot", true );

    octomapDisplay_ = manager_->createDisplay( "rviz/MarkerArray", "Octomap view", true );
    ROS_ASSERT( octomapDisplay_ != NULL );

    octomapDisplay_->subProp( "Marker Topic" )->setValue(octomapTopic_);

//    toolManager_ = manager_->getToolManager();
//    measureTool_ = toolManager_->addTool("rviz/Measure");
//    toolManager_->setCurrentTool(measureTool_);

    /*
    //Image :
        grid_ = manager_->createDisplay( "rviz/Image", "Image View", true );
        ROS_ASSERT( grid_ != NULL );
        grid_->subProp( "Image Topic" )->setValue( "/camera/rgb/image_raw" );
        grid_->subProp( "Transport Hint" )->setValue( "theora" );


    //Depth Cloud :
        grid_ = manager_->createDisplay( "rviz/DepthCloud", "Image View", true );
        ROS_ASSERT( grid_ != NULL );

        grid_->subProp( "Depth Map Topic" )->setValue( "/camera/depth/image_raw" );
        grid_->subProp( "Depth Map Transport Hint" )->setValue( "raw" );
        grid_->subProp( "Color Image Topic" )->setValue( "/camera/rgb/image_raw" );
        grid_->subProp( "Color Transport Hint" )->setValue( "raw" );
        grid_->subProp("Queue Size")->setValue(5);
        grid_->subProp("Style")->setValue("Flat Squares");

    //PointCloud2 :
    mainDisplay_ = manager_->createDisplay( "rviz/PointCloud2", "3D Pointcloud view", true );
    ROS_ASSERT( mainDisplay_ != NULL );

    mainDisplay_->subProp( "Topic" )->setValue( "/camera/depth/points" );
    mainDisplay_->subProp( "Selectable" )->setValue( "true" );
    mainDisplay_->subProp( "Style" )->setValue( "Boxes" );
    mainDisplay_->subProp("Alpha")->setValue(1);
    manager_->createDisplay( "rviz/Grid", "Grid", true );

    //MarkerArray :
    rviz::Display* octomapDisplay_ = manager_->createDisplay( "rviz/MarkerArray", "Octomap view", true );
    ROS_ASSERT( octomapDisplay_ != NULL );

    octomapDisplay_->subProp( "Marker Topic" )->setValue( "/occupied_cells_vis_array" );


*/

}

void FallRiskGUI::keyPressEvent(QKeyEvent *event)
{
    switch(event->key())
    {
    case Qt::Key_W:
        moveBaseForward();
        //        sendMoveBaseCmd();
        ROS_INFO("key W pressed");
        break;
    case Qt::Key_A:
        moveBaseLeft();
        //        sendMoveBaseCmd();
        ROS_INFO("key A pressed");
        break;
    case Qt::Key_D:
        moveBaseRight();
        //        sendMoveBaseCmd();
        ROS_INFO("key D pressed");
        break;
    case Qt::Key_S:
        moveBaseBackward();
        //        sendMoveBaseCmd();
        ROS_INFO("key S pressed");
        break;
        //    case Qt::Key_Q:
        //        move_in();
        //        break;
        //    case Qt::Key_E:
        //        move_out();
        //        break;
    default:
        QWidget::keyPressEvent(event);
        break;
    }
}

void FallRiskGUI::distanceSubCallback(const std_msgs::Float32::ConstPtr& msg)
{
    ////    ROS_INFO("distance: %f",msg->data);
    //    QLocale german(QLocale::German, QLocale::Germany);
    //    QString qdist = german.toString(msg->data, 'f', 2);
    //    ui->lbDistance->setText(qdist);
}

void FallRiskGUI::baseStatusCheck(const kobuki_msgs::SensorState::ConstPtr& msg)
{
    /*---------- battery of kobuki base -----------*/
    //    ROS_INFO("battery: %d",msg->battery);

    int battery_percentage = 0;

    battery_percentage = (msg->battery - BASE_BATTERY_DANGER)*100/(BASE_BATTERY_CAP-BASE_BATTERY_DANGER);
    ui->pbBaseBattery->setValue(battery_percentage);
    //    if(msg->battery <= BASE_BATTERY_LOW)
    //    QPalette p = ui->pbBaseBattery->palette();
    //        p.setColor(QPalette::Highlight, Qt::red);
    //        ui->pbBaseBattery->setPalette(p);
    //    else
    //ui->pbBaseBattery->setStyleSheet(safe);

    /*-------------- bumper sensors ---------------*/
    if(msg->bumper == msg->BUMPER_LEFT)
    {
        ROS_INFO("BUMPER LEFT");
        ui->lbBumperLeft->setStyleSheet("QLabel { background-color : rgb(255, 0, 0); color : rgb(255, 255, 255); }");
    }
    else if(msg->bumper == msg->BUMPER_CENTRE)
    {
        ROS_INFO("BUMPER CENTER");
        ui->lbBumperCenter->setStyleSheet("QLabel { background-color : rgb(255, 0, 0); color : rgb(255, 255, 255); }");
    }
    else if(msg->bumper == msg->BUMPER_RIGHT)
    {
        ROS_INFO("BUMPER RIGHT");
        ui->lbBumperRight->setStyleSheet("QLabel { background-color : rgb(255, 0, 0); color : rgb(255, 255, 255); }");
    }
    else
    {
        ui->lbBumperCenter->setStyleSheet("QLabel { background-color : rgb(0, 204, 102); color : rgb(255, 255, 255); }");
        ui->lbBumperLeft->setStyleSheet("QLabel { background-color : rgb(0, 204, 102); color : rgb(255, 255, 255); }");
        ui->lbBumperRight->setStyleSheet("QLabel { background-color : rgb(0, 204, 102); color : rgb(255, 255, 255); }");
    }

    /*------------ wheel drop sensors -------------*/
    if(msg->wheel_drop == msg->WHEEL_DROP_LEFT)
    {
        ROS_INFO("wheel drop left");
        ui->lbWheelLeft->setStyleSheet("QLabel { background-color : rgb(255, 0, 0); color : rgb(255, 255, 255); }");
    }
    else if(msg->wheel_drop == msg->WHEEL_DROP_RIGHT)
    {
        ROS_INFO("wheel drop right");
        ui->lbWheelRight->setStyleSheet("QLabel { background-color : rgb(255, 0, 0); color : rgb(255, 255, 255); }");
    }
    else if(msg->wheel_drop == ( msg->WHEEL_DROP_LEFT+msg->WHEEL_DROP_RIGHT))
    {
        ROS_INFO("wheel drop both");
        ui->lbWheelLeft->setStyleSheet("QLabel { background-color : rgb(255, 0, 0); color : rgb(255, 255, 255); }");
        ui->lbWheelRight->setStyleSheet("QLabel { background-color : rgb(255, 0, 0); color : rgb(255, 255, 255); }");
    }
    else
    {
        ui->lbWheelLeft->setStyleSheet("QLabel { background-color : rgb(0, 204, 102); color : rgb(255, 255, 255); }");
        ui->lbWheelRight->setStyleSheet("QLabel { background-color : rgb(0, 204, 102); color : rgb(255, 255, 255); }");
    }

    /*-------------- cliff sensors ---------------*/
    if(msg->cliff == msg->CLIFF_LEFT)
    {
        ROS_INFO("cliff left");
        ui->lbCliffLeft->setStyleSheet("QLabel { background-color : rgb(255, 0, 0); color : rgb(255, 255, 255); }");
    }
    else if(msg->cliff == msg->CLIFF_CENTRE)
    {
        ROS_INFO("cliff center");
        ui->lbCliffCenter->setStyleSheet("QLabel { background-color : rgb(255, 0, 0); color : rgb(255, 255, 255); }");
    }
    else if(msg->cliff == msg->CLIFF_RIGHT)
    {
        ROS_INFO("cliff right");
        ui->lbCliffRight->setStyleSheet("QLabel { background-color : rgb(255, 0, 0); color : rgb(255, 255, 255); }");
    }
    else
    {
        ui->lbCliffCenter->setStyleSheet("QLabel { background-color : rgb(0, 204, 102); color : rgb(255, 255, 255); }");
        ui->lbCliffLeft->setStyleSheet("QLabel { background-color : rgb(0, 204, 102); color : rgb(255, 255, 255); }");
        ui->lbCliffRight->setStyleSheet("QLabel { background-color : rgb(0, 204, 102); color : rgb(255, 255, 255); }");
    }
}

void FallRiskGUI::liveVideoCallback(const sensor_msgs::ImageConstPtr& msg)
{

    cv_bridge::CvImagePtr cv_ptr;
    try
    {
        cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR16);
    }
    catch (cv_bridge::Exception& e)
    {
        ROS_ERROR("cv_bridge exception: %s", e.what());
        return;
    }
    //  convert cv image into RGB image and resize it to the size of available layout
    cv::Mat RGBImg;
    QLabel* liveVideoLabel = ui->liveVideoLabel;

    int height = liveVideoLabel->height();
    int width =  liveVideoLabel->width();

    if(liveVideoLabel->height() > liveVideoLabel->width()*3/4)
        height= liveVideoLabel->width()*3/4;
    else
        width = liveVideoLabel->height()*4/3;

    cv::cvtColor(cv_ptr->image,RGBImg,CV_BGR2RGB);
    cv::resize(RGBImg,RGBImg,cvSize(width,height));

    //  convert RGB image into QImage and publish that on the label for livevideo
    QImage qImage_= QImage((uchar*) RGBImg.data, RGBImg.cols, RGBImg.rows, RGBImg.cols*3, QImage::Format_RGB888);
    liveVideoLabel->setPixmap(QPixmap::fromImage(qImage_));
    liveVideoLabel->show();
}


void FallRiskGUI::setRobotVelocity()
{
    linearVelocity = ui->sliderLinearVel->value()*(LIN_VEL_MAX-LIN_VEL_MIN)/100+LIN_VEL_MIN;
    ROS_INFO("Linear velocity:%f",linearVelocity);
    angularVelocity = ui->sliderAngularVel->value()*(ANG_VEL_MAX-ANG_VEL_MIN)/100+ANG_VEL_MIN;
    ROS_INFO("Angular velocity:%f",angularVelocity);
}

void FallRiskGUI::moveBaseForward()
{
    ROS_INFO("move forward");

    moveBaseCmd.linear.x=linearVelocity;
    moveBaseCmd.linear.y=0;
    moveBaseCmd.linear.z=0;

    moveBaseCmd.angular.x=0;
    moveBaseCmd.angular.y=0;
    moveBaseCmd.angular.z=0;

    sendMoveBaseCmd();
}

void FallRiskGUI::moveBaseBackward()
{
    ROS_INFO("move backward");

    moveBaseCmd.linear.x=-linearVelocity;
    moveBaseCmd.linear.y=0;
    moveBaseCmd.linear.z=0;

    moveBaseCmd.angular.x=0;
    moveBaseCmd.angular.y=0;
    moveBaseCmd.angular.z=0;

    sendMoveBaseCmd();
}

void FallRiskGUI::moveBaseLeft()
{
    ROS_INFO("move left");

    moveBaseCmd.linear.x=0;
    moveBaseCmd.linear.y=0;
    moveBaseCmd.linear.z=0;

    moveBaseCmd.angular.x=0;
    moveBaseCmd.angular.y=0;
    moveBaseCmd.angular.z=angularVelocity;

    sendMoveBaseCmd();
}

void FallRiskGUI::moveBaseRight()
{
    ROS_INFO("move right");

    moveBaseCmd.linear.x=0;
    moveBaseCmd.linear.y=0;
    moveBaseCmd.linear.z=0;

    moveBaseCmd.angular.x=0;
    moveBaseCmd.angular.y=0;
    moveBaseCmd.angular.z=-angularVelocity;

    sendMoveBaseCmd();
}

void FallRiskGUI::sendMoveBaseCmd()
{
    if(ros::ok() && moveBaseCmdPub)
    {
        moveBaseCmdPub.publish(moveBaseCmd);
        ROS_INFO("move base cmd sent");
    }
}


void FallRiskGUI::initTools(){
    toolManager_ = manager_->getToolManager();

    pointTool_ = toolManager_->addTool("rviz/PublishPoint");
    measureTool_ = toolManager_->addTool("rviz/Measure");
    setGoal_ = toolManager_->addTool("rviz/SetGoal");
    setInitialPose_=toolManager_->addTool("rviz/SetInitialPose");
    interactTool_ = toolManager_->addTool("rviz/Interact");

}

void FallRiskGUI::setCurrentTool(int btnID)
{
    if(btnID == -2)
    {
        ROS_INFO("Interact Tool Selected");
    }
    else if(btnID == -3)
    {
        ROS_INFO("Measure Tool Selected");
    }
    else if(btnID == -4)
    {
        ROS_INFO("2DPoseEstimate Tool Selected");
    }
    else if(btnID == -5)
    {
        ROS_INFO("2DNavGoal Tool Selected");
    }
    else if(btnID == -6)
    {
        ROS_INFO("PublishPoint Tool Selected");
    }

//    ROS_INFO("ID:%d",btnID);

//    ROS_INFO("Measurement Tool Selected");
//    toolManager_->setCurrentTool(measureTool_);
//    toolManager_ = manager_->getToolManager();
//    measureTool_ = toolManager_->addTool("rviz/Measure");
//    toolManager_->setCurrentTool(measureTool_);

}

