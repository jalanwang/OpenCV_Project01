#ifndef KIM_FACE_HPP
#define KIM_FACE_HPP

#include <opencv2/opencv.hpp>
#include <iostream>

class KimFace {
private:
    cv::Mat originalImage;
    cv::Mat currentImage;
    int hitCount;

public:
    KimFace() : hitCount(0) {}

    bool load(const std::string& path) {
        originalImage = cv::imread(path);
        if (originalImage.empty()) {
            std::cerr << "Error: Could not load image " << path << std::endl;
            return false;
        }
        currentImage = originalImage.clone();
        return true;
    }

    // 맞았을 때 호출: 피부색(색상) 변경
    void hit() {
        hitCount++;
        if (currentImage.empty()) return;

        // HSV 색공간으로 변환하여 색상(Hue) 변경
        cv::Mat hsv;
        cv::cvtColor(currentImage, hsv, cv::COLOR_BGR2HSV);

        // H(Hue) 채널에 값을 더해 색상을 회전시킴
        for (int y = 0; y < hsv.rows; y++) {
            for (int x = 0; x < hsv.cols; x++) {
                cv::Vec3b& pixel = hsv.at<cv::Vec3b>(y, x);
                // 30씩 색상 변경 (점점 변함)
                pixel[0] = (pixel[0] + 30) % 180; 
            }
        }

        cv::cvtColor(hsv, currentImage, cv::COLOR_HSV2BGR);
    }

    // 화면에 얼굴 그리기
    void draw(cv::Mat& frame, cv::Point center, int radius) {
        if (currentImage.empty()) {
            // 이미지가 없으면 그냥 빨간 원 그리기 (fallback)
            cv::circle(frame, center, radius, cv::Scalar(0, 0, 255), -1);
            return;
        }

        // 1. 얼굴 이미지를 공 크기에 맞게 리사이즈 (지름 = radius * 2)
        cv::Mat resized;
        cv::resize(currentImage, resized, cv::Size(radius * 2, radius * 2));

        // 2. 원형 마스크 생성
        cv::Mat mask = cv::Mat::zeros(resized.size(), CV_8UC1);
        cv::circle(mask, cv::Point(radius, radius), radius, cv::Scalar(255), -1);

        // 3. 프레임 내의 그릴 영역(ROI) 계산
        int x1 = center.x - radius;
        int y1 = center.y - radius;
        
        // 화면 밖으로 나가는 경우 예외 처리
        if (x1 < 0 || y1 < 0 || x1 + resized.cols > frame.cols || y1 + resized.rows > frame.rows) {
            return; 
        }

        cv::Rect roiRect(x1, y1, resized.cols, resized.rows);
        cv::Mat roi = frame(roiRect);

        // 4. 마스크를 사용하여 원형으로 이미지 복사
        resized.copyTo(roi, mask);
    }
    
    bool isLoaded() const {
        return !currentImage.empty();
    }
};

#endif // KIM_FACE_HPP
