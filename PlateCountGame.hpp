#ifndef PLATE_COUNT_GAME_HPP
#define PLATE_COUNT_GAME_HPP

#include "GameStrategy.hpp"
#include "WebcamManager.hpp"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>

class PlateCountGame : public GameStrategy {
private:
    WebcamManager& webcam;
    cv::Mat plateTemplate;

public:
    PlateCountGame(WebcamManager& wm) : webcam(wm) {}

    virtual GameState run() override {
        std::cout << "Starting Plate Count Game..." << std::endl;
        
        // Load the template image
        plateTemplate = cv::imread("com_plate.jpg");
        cv::Mat binaryPattern;

        if (plateTemplate.empty()) {
            std::cerr << "Error: com_plate.jpg not found!" << std::endl;
        } else {
            cv::Mat gray;
            cv::cvtColor(plateTemplate, gray, cv::COLOR_BGR2GRAY);
            cv::threshold(gray, binaryPattern, 128, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
        }

        bool isResultMode = false;
        cv::Mat capturedFrame;
        int capturedCount = 0;

        while (true) {
            if (!isResultMode) {
                // Preview Mode
                cv::Mat frame;
                if (!webcam.getFrame(frame)) return GameState::EXIT;
                cv::flip(frame, frame, 1); // Mirror view

                // Define ROI (500x500 centered)
                int roiSize = 500;
                int x = std::max(0, (frame.cols - roiSize) / 2);
                int y = std::max(0, (frame.rows - roiSize) / 2);
                int w = std::min(roiSize, frame.cols - x);
                int h = std::min(roiSize, frame.rows - y);
                cv::Rect roiRect(x, y, w, h);

                // Draw ROI box
                cv::rectangle(frame, roiRect, cv::Scalar(0, 255, 255), 2);
                
                // Display binary pattern thumbnail
                if (!binaryPattern.empty()) {
                    cv::Mat smallPattern;
                    cv::resize(binaryPattern, smallPattern, cv::Size(100, 100));
                    cv::cvtColor(smallPattern, smallPattern, cv::COLOR_GRAY2BGR);
                    
                    cv::Rect patternRoi(frame.cols - 110, 10, 100, 100);
                    if (patternRoi.x >= 0 && patternRoi.y >= 0 && 
                        patternRoi.x + patternRoi.width <= frame.cols && 
                        patternRoi.y + patternRoi.height <= frame.rows) {
                        smallPattern.copyTo(frame(patternRoi));
                        cv::rectangle(frame, patternRoi, cv::Scalar(0, 255, 0), 1);
                        cv::putText(frame, "Binary", cv::Point(frame.cols - 110, 125), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(0, 255, 0), 1);
                    }
                }

                cv::putText(frame, "Place objects in box", cv::Point(20, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);
                cv::putText(frame, "Press SPACE to Capture", cv::Point(20, 60), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);

                cv::imshow("GAME", frame);

                int key = cv::waitKey(10);
                if (key == 27) return GameState::EXIT;
                if (key == 32) { // Space to Capture
                    capturedFrame = frame(roiRect).clone();
                    
                    // Analyze captured frame
                    if (!binaryPattern.empty()) {
                        cv::Mat grayFrame, binaryFrame;
                        cv::cvtColor(capturedFrame, grayFrame, cv::COLOR_BGR2GRAY);
                        cv::threshold(grayFrame, binaryFrame, 128, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
                        
                        cv::Mat result;
                        capturedCount = 0;
                        
                        if (binaryPattern.cols <= binaryFrame.cols && binaryPattern.rows <= binaryFrame.rows) {
                            cv::matchTemplate(binaryFrame, binaryPattern, result, cv::TM_CCORR_NORMED);
                            double threshold = 0.8;
                            
                            while (true) {
                                double minVal, maxVal;
                                cv::Point minLoc, maxLoc;
                                cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);

                                if (maxVal >= threshold) {
                                    capturedCount++;
                                    cv::rectangle(capturedFrame, maxLoc, cv::Point(maxLoc.x + binaryPattern.cols, maxLoc.y + binaryPattern.rows), cv::Scalar(0, 255, 255), 2);
                                    
                                    int startX = std::max(0, maxLoc.x - binaryPattern.cols / 2);
                                    int startY = std::max(0, maxLoc.y - binaryPattern.rows / 2);
                                    int endX = std::min(result.cols, maxLoc.x + binaryPattern.cols / 2);
                                    int endY = std::min(result.rows, maxLoc.y + binaryPattern.rows / 2);
                                    
                                    cv::rectangle(result, cv::Point(startX, startY), cv::Point(endX, endY), cv::Scalar(0), -1);
                                } else {
                                    break;
                                }
                                if (capturedCount > 50) break;
                            }
                        }
                    }
                    isResultMode = true;
                }
            } else {
                // Result Mode
                cv::Mat displayFrame = capturedFrame.clone();
                cv::putText(displayFrame, "Count: " + std::to_string(capturedCount), cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.5, cv::Scalar(0, 0, 255), 3);
                cv::putText(displayFrame, "Press 'R' to Retry", cv::Point(20, displayFrame.rows - 20), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
                
                cv::imshow("GAME", displayFrame);
                
                int key = cv::waitKey(10);
                if (key == 27) return GameState::EXIT;
                if (key == 'r' || key == 'R') {
                    isResultMode = false;
                }
            }
        }
        return GameState::EXIT;
    }
};

#endif // PLATE_COUNT_GAME_HPP
