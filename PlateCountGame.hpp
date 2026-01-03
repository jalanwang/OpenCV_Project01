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
        if (plateTemplate.empty()) {
            std::cerr << "Error: com_plate.jpg not found!" << std::endl;
        }

        while (true) {
            cv::Mat frame;
            if (!webcam.getFrame(frame)) return GameState::EXIT;
            cv::flip(frame, frame, 1); // Mirror view

            int count = 0;

            if (!plateTemplate.empty()) {
                cv::Mat result;
                // Ensure template is smaller than frame
                if (plateTemplate.cols <= frame.cols && plateTemplate.rows <= frame.rows) {
                    cv::matchTemplate(frame, plateTemplate, result, cv::TM_CCOEFF_NORMED);
                    
                    double threshold = 0.8;
                    
                    while (true) {
                        double minVal, maxVal;
                        cv::Point minLoc, maxLoc;
                        cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);

                        if (maxVal >= threshold) {
                            count++;
                            cv::rectangle(frame, maxLoc, cv::Point(maxLoc.x + plateTemplate.cols, maxLoc.y + plateTemplate.rows), cv::Scalar(0, 255, 255), 2);
                            
                            // Mask out the detected area in the result matrix to avoid re-detecting the same object
                            // We mask out a region around the center of the match
                            int w = plateTemplate.cols;
                            int h = plateTemplate.rows;
                            
                            // Calculate masking region in result matrix coordinates
                            int startX = std::max(0, maxLoc.x - w / 2);
                            int startY = std::max(0, maxLoc.y - h / 2);
                            int endX = std::min(result.cols, maxLoc.x + w / 2);
                            int endY = std::min(result.rows, maxLoc.y + h / 2);
                            
                            // Fill with -1 (lowest possible value for TM_CCOEFF_NORMED)
                            cv::rectangle(result, cv::Point(startX, startY), cv::Point(endX, endY), cv::Scalar(-1.0), -1);
                        } else {
                            break;
                        }
                        
                        if (count > 50) break; // Safety limit
                    }
                } else {
                     cv::putText(frame, "Template larger than frame", cv::Point(50, 50), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
                }
            } else {
                cv::putText(frame, "com_plate.jpg NOT FOUND", cv::Point(50, 50), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
            }

            cv::putText(frame, "Plate Count: " + std::to_string(count), cv::Point(20, 50),
                cv::FONT_HERSHEY_SIMPLEX, 1.5, cv::Scalar(0, 255, 0), 3);

            cv::imshow("GAME", frame);

            int key = cv::waitKey(10);
            if (key == 27) return GameState::EXIT;
        }
        return GameState::EXIT;
    }
};

#endif // PLATE_COUNT_GAME_HPP
