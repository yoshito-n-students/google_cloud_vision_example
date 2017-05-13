#include <ros/console.h>
#include <ros/init.h>
#include <ros/node_handle.h>
#include <ros/param.h>
#include <ros/service.h>

#include <google_cloud_vision_msgs/drawing.hpp>
#include <google_cloud_vision_msgs/encode.hpp>
#include <google_cloud_vision_srvs/Annotate.h>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

namespace msgs = google_cloud_vision_msgs;
namespace rp = ros::param;
namespace rs = ros::service;
namespace srvs = google_cloud_vision_srvs;

int main(int argc, char *argv[]) {
  // init ROS
  ros::init(argc, argv, "annotate_example");
  ros::NodeHandle nh;

  // load parameters
  const std::string image_path(rp::param< std::string >("~image", ""));
  const std::string type(rp::param< std::string >("~type", msgs::Feature::FACE_DETECTION));
  const int max_results(rp::param("~max_results", 0));
  const int http_timeout(rp::param("~http_timeout", 60));
  if (image_path.empty()) {
    ROS_ERROR("A required parameter ~image is missing");
    return 1;
  }

  // wait for the service
  if (!rs::waitForService("annotate")) {
    ROS_ERROR("No annotate service");
    return 1;
  }

  // open the image file
  const cv::Mat image(cv::imread(image_path));
  if (image.empty()) {
    ROS_ERROR_STREAM("Could not open " << image_path);
    return 1;
  }

  // create a request
  srvs::Annotate::Request request;
  request.requests.resize(1);
  msgs::encode(image, request.requests[0].image);
  request.requests[0].features.resize(1);
  request.requests[0].features[0].type = type;
  if (max_results > 0) {
    request.requests[0].features[0].has_max_results = true;
    request.requests[0].features[0].max_results = max_results;
  }
  request.http_timeout = http_timeout;

  // call the service
  srvs::Annotate::Response response;
  if (!rs::call("annotate", request, response)) {
    ROS_ERROR("Failed to call the service");
    return 1;
  }
  if (response.responses.size() != 1) {
    ROS_ERROR_STREAM("Invalid response size: " << response.responses.size());
    return 1;
  }
  if (response.responses[0].has_error) {
    ROS_ERROR_STREAM("Error response:\n" << response.responses[0].error);
    return 1;
  }

  // show the result
  ROS_INFO_STREAM("Response:\n" << response);
  if (type == msgs::Feature::FACE_DETECTION) {
    cv::Mat image_out(image.clone());
    msgs::drawFaceAnnotations(image_out, response.responses[0].face_annotations,
                              CV_RGB(255, 0, 255), 2);
    cv::imshow("face_annotations", image_out);
    cv::waitKey();
  } else if (type == msgs::Feature::TEXT_DETECTION) {
    cv::Mat image_out(image.clone());
    msgs::drawTextAnnotations(image_out, response.responses[0].text_annotations, 0.4,
                              CV_RGB(255, 255, 255), CV_RGB(255, 0, 255), 1);
    cv::imshow("text_annotations", image_out);
    cv::waitKey();
  }

  return 0;
}
