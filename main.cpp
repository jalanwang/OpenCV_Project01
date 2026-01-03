#include "opencv2/opencv.hpp"
#include <iostream>

#include <memory>
#include "WebcamManager.hpp"
#include "GameStrategy.hpp"
#include "RedBallGame.hpp"
#include "TransformGame.hpp"

int main()
{
    std::cout << "Hello World! CV Version: " << CV_VERSION << std::endl;

    WebcamManager webcam;
    if (!webcam.initialize()) {
        return -1;
    }

    // 전략 패턴 사용: RedBallGame 선택
    std::unique_ptr<GameStrategy> game = std::make_unique<RedBallGame>(webcam);
    game->run();

    return 0;
}

