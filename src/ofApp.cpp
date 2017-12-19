#include "ofApp.h"

using namespace cv;

//--------------------------------------------------------------
void ofApp::setup(){

  ofSetVerticalSync(true);
  frameByframe = false;
  bHide = false;
  
  gui.setup(); // most of the time you don't need a name
  
  gui.add(sigma.setup("Sigma (x, y)", ofVec2f(15, 10), ofVec2f(0, 0), ofVec2f(100, 100)));
  gui.add(center.setup("Center (x, y)", ofVec2f(30, 25), ofVec2f(0, 0), ofVec2f(100, 100)));
  gui.add(threnum.setup("Threshold", 128, 0, 255));
  gui.add(elementnum.setup("Element Size", 6, 0, 15));
  gui.add(morphologyIteration.setup("Iteration", 2, 0, 15));
  
  eyeVideo.load("movies/MyMovie.mp4");

  
  eyeVideo.setLoopState(OF_LOOP_NORMAL);
  eyeVideo.play();
}

//--------------------------------------------------------------
void ofApp::update(){
  eyeVideo.update();
  
  Mat eyeMat = ofxCv::toCv(eyeVideo);
  // グレースケールに変換する
  cvtColor(eyeMat, gray_eyeMat, CV_RGB2GRAY);
  mask = createMask(sigma->x, sigma->y, gray_eyeMat.cols/2, gray_eyeMat.rows, center->x, center->y);
  add(gray_eyeMat, mask, eyeOnly);
  threshold(eyeOnly, thre_img, threnum, 255, 0);
  
  Mat element = getStructuringElement(cv::MORPH_RECT,
                                          cv::Size(2 * elementnum + 1, 2 * elementnum + 1),
                                          cv::Point(elementnum, elementnum));
  erode(thre_img, Iteration_img, element, cv::Point(-1, -1), morphologyIteration);
  dilate(Iteration_img, Iteration_img, element, cv::Point(-1, -1), morphologyIteration);
  
//  threshold(Iteration_img, Iterationimg, 0.0, 255.0, CV_THRESH_BINARY_INV | CV_THRESH_OTSU);
  Mat Iterationimg;
  threshold(Iteration_img, Iterationimg, 0.0, 255.0, CV_THRESH_BINARY | CV_THRESH_OTSU);
 
  
  circle_img = processImage(Iterationimg);
  
}

//--------------------------------------------------------------
void ofApp::draw(){
  ofSetHexColor(0xFFFFFF);
  
  eyeVideo.draw(20, 20);

  ofxCv::drawMat(mask, 20, 70);
  ofxCv::drawMat(eyeOnly, 20, 120);
  ofxCv::drawMat(thre_img, 20, 170);
  ofxCv::drawMat(Iteration_img, 20, 220);
  ofxCv::drawMat(circle_img, 20, 270);
  
  if(!bHide){
    gui.draw();
  }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
  switch(key){
    case 'h':
      if(key == 'h'){
        bHide = !bHide;
      }
      break;
    case 'f':
      frameByframe=!frameByframe;
      eyeVideo.setPaused(frameByframe);
      break;
    case OF_KEY_LEFT:
      eyeVideo.previousFrame();
      break;
    case OF_KEY_RIGHT:
      eyeVideo.nextFrame();
      break;
    case '0':
      eyeVideo.firstFrame();
      break;
  }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
  
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
  if(!frameByframe){
    int width = ofGetWidth();
    float pct = (float)x / (float)width;
    float speed = (2 * pct - 1) * 5.0f;
    eyeVideo.setSpeed(speed);
  }
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
  if(!frameByframe){
    int width = ofGetWidth();
    float pct = (float)x / (float)width;
    eyeVideo.setPosition(pct);
  }
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
  if(!frameByframe){
    eyeVideo.setPaused(true);
  }
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
  if(!frameByframe){
    eyeVideo.setPaused(false);
  }
}
//--------------------------------------------------------------
Mat ofApp::createMask(float sx, float sy, int width, int height, double centerX, double centerY)
{
  Mat eye(height, width, CV_8UC1);
  Mat kernel(height, width, CV_64F);

  // Calculate the normal distribution
  for (int i = 0; i < height; i++)
    {
      for (int j = 0; j < width; j++)
        {
          double x = (double)j - centerX;
          double y = (double)i - centerY;
          double value = (1.0 / (2.0 * PI*pow(sx,2)*pow(sy,2))) * exp( (-0.5) * ( pow(x, 2) / pow(sx, 2) + pow(y, 2) / pow(sy, 2)));
          kernel.at<double>(i, j) = value;
          }
     }

  // Normalize
  normalize(kernel, eye, 0, 255, cv::NORM_MINMAX, CV_8UC1);
  hconcat(eye, eye, eye);
  
  // Invert color
  bitwise_not(eye, eye);
  
  return eye;
}

//--------------------------------------------------------------
Mat ofApp::processImage(Mat img){
  
  vector<vector<cv::Point>> contours;
  findContours(img, contours, RETR_LIST, CHAIN_APPROX_NONE);
  Mat cimage = Mat::zeros(img.size(), CV_8UC3);
  
  for(size_t i = 0; i < contours.size(); i++)
  {
    if (contours[i][0].x == 1 || contours[i][0].y == 1)
      continue;
    
    size_t count = contours[i].size();
    if(count < 6)
      continue;
  
    Mat pointsf;
    Mat(contours[i]).convertTo(pointsf, CV_32F);
    RotatedRect box = fitEllipse(pointsf);
    if( MAX(box.size.width, box.size.height) > MIN(box.size.width, box.size.height)*30 )
      continue;
    
    drawContours(cimage, contours, (int)i, Scalar::all(255), 1, 8);
    
    ellipse(cimage, box, cvScalar(0,0,255), 1, CV_AA);
    ellipse(cimage, box.center, box.size*0.5f, box.angle, 0, 360, cvScalar(0.0,255.0,255.0), 1, CV_AA);
//    Point2f vtx[4];
//    box.points(vtx);
    
//    for( int j = 0; j < 4; j++ )
//      line(cimage, vtx[j], vtx[(j+1)%4], cvScalar(0.0,255.0,0.0), 1, CV_AA);
  }
  
  return cimage;
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){
  
}
//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){
  
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
  
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){
  
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 
  
}
