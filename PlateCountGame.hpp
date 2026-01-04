#ifndef PLATE_COUNT_GAME_HPP
#define PLATE_COUNT_GAME_HPP

#include "GameStrategy.hpp"
#include "WebcamManager.hpp"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>

class PlateCountGame : public GameStrategy {
private:
    WebcamManager& webcam;

public:
    PlateCountGame(WebcamManager& wm) : webcam(wm) {}

    virtual GameState run() override {
        std::cout << "Starting Plate Count Game (Template Matching)..." << std::endl;
        
        // Load images as grayscale
        cv::Mat templateImg = cv::imread("com_plate1.jpg", cv::IMREAD_GRAYSCALE);
        cv::Mat sceneImg = cv::imread("plate_block.jpg", cv::IMREAD_GRAYSCALE);
        
        if (templateImg.empty()) {
            std::cerr << "Error: com_plate1.jpg not found!" << std::endl;
            return GameState::EXIT;
        }
        
        // Increase brightness of template
        templateImg = templateImg + 50;

        if (sceneImg.empty()) {
            std::cerr << "Error: plate_block.jpg not found!" << std::endl;
            sceneImg = cv::Mat::zeros(480, 640, CV_8UC1);
            cv::putText(sceneImg, "plate_block.jpg NOT FOUND", cv::Point(50, 240), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255), 2);
        } else {
            // Preprocessing: Brightness + 50, Gaussian Blur
            sceneImg = sceneImg + 50;
            cv::GaussianBlur(sceneImg, sceneImg, cv::Size(5, 5), 0);
        }

        // 1. Template Matching
        cv::Mat result;
        cv::matchTemplate(sceneImg, templateImg, result, cv::TM_CCOEFF_NORMED);

        // 2. Threshold and NMS
        double threshold = 0.4;
        std::vector<cv::Point> locations;
        std::vector<float> scores;
        
        // Find all locations above threshold
        for(int y = 0; y < result.rows; y++) {
            for(int x = 0; x < result.cols; x++) {
                float score = result.at<float>(y, x);
                if (score >= threshold) {
                    locations.push_back(cv::Point(x, y));
                    scores.push_back(score);
                }
            }
        }

        // Simple NMS
        std::vector<cv::Rect> detectedRects;
        std::vector<float> detectedScores;
        int w = templateImg.cols;
        int h = templateImg.rows;

        // Sort by score descending
        std::vector<int> indices(scores.size());
        std::iota(indices.begin(), indices.end(), 0);
        std::sort(indices.begin(), indices.end(), [&](int a, int b) {
            return scores[a] > scores[b];
        });

        for (int idx : indices) {
            cv::Point pt = locations[idx];
            cv::Rect rect(pt.x, pt.y, w, h);
            bool overlap = false;
            for (const auto& existing : detectedRects) {
                cv::Rect intersection = rect & existing;
                if (intersection.area() > (rect.area() * 0.3)) { // 30% overlap threshold
                    overlap = true;
                    break;
                }
            }
            if (!overlap) {
                detectedRects.push_back(rect);
                detectedScores.push_back(scores[idx]);
            }
        }

        int count = detectedRects.size();
        cv::Mat resultImg;
        cv::cvtColor(sceneImg, resultImg, cv::COLOR_GRAY2BGR);
        
        // Draw Detections
        for (size_t i = 0; i < detectedRects.size(); i++) {
            cv::rectangle(resultImg, detectedRects[i], cv::Scalar(0, 255, 0), 2);
            cv::putText(resultImg, std::to_string(detectedScores[i]).substr(0, 4), detectedRects[i].tl(), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(0, 255, 0), 1);
        }

        // 4. Prepare Display
        // Resize template preserving aspect ratio
        double scale = std::min(100.0 / templateImg.cols, 100.0 / templateImg.rows);
        cv::Size newSize(int(templateImg.cols * scale), int(templateImg.rows * scale));
        cv::Mat smallTemplate;
        cv::resize(templateImg, smallTemplate, newSize);
        cv::cvtColor(smallTemplate, smallTemplate, cv::COLOR_GRAY2BGR);
        
        // Removed forced resize of resultImg to preserve aspect ratio

        cv::Rect templateRoi(10, 10, newSize.width, newSize.height);
        if (templateRoi.x + templateRoi.width <= resultImg.cols && templateRoi.y + templateRoi.height <= resultImg.rows) {
             smallTemplate.copyTo(resultImg(templateRoi));
             cv::rectangle(resultImg, templateRoi, cv::Scalar(255, 255, 255), 1);
             cv::putText(resultImg, "Template", cv::Point(10, 125), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255), 1);
        }
        
        cv::putText(resultImg, "Count: " + std::to_string(count), cv::Point(10, 150), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);

        cv::namedWindow("GAME");
        cv::moveWindow("GAME", 0, 0);

        while (true) {
            cv::imshow("GAME", resultImg);
            int key = cv::waitKey(10);
            if (key == 27) return GameState::EXIT;
        }
        return GameState::EXIT;
    }
};

#endif // PLATE_COUNT_GAME_HPP
