# CMake 빌드 시스템의 최소 요구 버전을 3.10으로 설정합니다.
cmake_minimum_required(VERSION 3.10)

# 프로젝트의 이름을 'OpenCVProject'로 정의합니다.
project(OpenCVProject)

# 시스템에 설치된 OpenCV 패키지를 찾습니다. REQUIRED 옵션은 찾지 못하면 구성을 중단(에러 발생)합니다.
find_package(OpenCV REQUIRED)

# HelloWorld.cpp와 Code1.cpp 소스 파일을 컴파일하여 'main'이라는 이름의 실행 파일을 생성합니다.
add_executable(main main.cpp code.cpp)

# 위에서 찾은 OpenCV 라이브러리 정보(${OpenCV_LIBS})를 'main' 실행 파일에 링크(연결)합니다.
target_link_libraries(main ${OpenCV_LIBS})

# 단일 파일일 경우의 예시
# cmake_minimum_required(VERSION 3.10)
# project(OpenCV_Test)

# # OpenCV 패키지 찾기
# find_package(OpenCV REQUIRED)

# add_executable(main main.cpp)

# # 헤더 경로 및 라이브러리 링크
# target_include_directories(main PRIVATE ${OpenCV_INCLUDE_DIRS})
# target_link_libraries(main ${OpenCV_LIBS})