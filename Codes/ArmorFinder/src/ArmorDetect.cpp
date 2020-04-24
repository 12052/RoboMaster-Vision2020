#include "ArmorFinder.h"

using namespace std;
using namespace cv;
    /**
     * @author:任天锋
     * @desc:装甲板识别,整理自SJTU
     * @qq:1010645265
     */
    // 判断两个灯条的角度差
    static bool angelJudge(const LightBlob &light_blob_i, const LightBlob &light_blob_j) {
        float angle_i = light_blob_i.rect.size.width > light_blob_i.rect.size.height ? light_blob_i.rect.angle :
                        light_blob_i.rect.angle - 90;
        float angle_j = light_blob_j.rect.size.width > light_blob_j.rect.size.height ? light_blob_j.rect.angle :
                        light_blob_j.rect.angle - 90;
        return abs(angle_i - angle_j) < 10;
    }
    // 判断两个灯条的高度差
    static bool heightJudge(const LightBlob &light_blob_i, const LightBlob &light_blob_j) {
        cv::Point2f centers = light_blob_i.rect.center - light_blob_j.rect.center;
        return abs(centers.y) < 30;
    }
    // 判断两个灯条的间距
    static bool lengthJudge(const LightBlob &light_blob_i, const LightBlob &light_blob_j) {
        double side_length;
        cv::Point2f centers = light_blob_i.rect.center - light_blob_j.rect.center;
        side_length = sqrt(centers.ddot(centers));
        return (side_length / light_blob_i.length < 10 && side_length / light_blob_i.length > 0.5);
    }
    // 判断两个灯条的长度比
    static bool lengthRatioJudge(const LightBlob &light_blob_i, const LightBlob &light_blob_j) {
        return (light_blob_i.length / light_blob_j.length < 2.5
                && light_blob_i.length / light_blob_j.length > 0.4);
    }

    /* 判断两个灯条的错位度，不知道英文是什么！！！ */
    static bool CuoWeiDuJudge(const LightBlob &light_blob_i, const LightBlob &light_blob_j) {
        float angle_i = light_blob_i.rect.size.width > light_blob_i.rect.size.height ? light_blob_i.rect.angle :
                        light_blob_i.rect.angle - 90;
        float angle_j = light_blob_j.rect.size.width > light_blob_j.rect.size.height ? light_blob_j.rect.angle :
                        light_blob_j.rect.angle - 90;
        float angle = (angle_i + angle_j) / 2.0 / 180.0 * 3.14159265459;
        if (abs(angle_i - angle_j) > 90) {
            angle += 3.14159265459 / 2;
        }
        Eigen::Vector2f orientation(cos(angle), sin(angle));
        Eigen::Vector2f p2p(light_blob_j.rect.center.x - light_blob_i.rect.center.x,
                    light_blob_j.rect.center.y - light_blob_i.rect.center.y);
        return abs(orientation.dot(p2p)) < 25;
    }
    // 判断装甲板方向
    static bool boxAngleJudge(const LightBlob &light_blob_i, const LightBlob &light_blob_j) {
        float angle_i = light_blob_i.rect.size.width > light_blob_i.rect.size.height ? light_blob_i.rect.angle :
                        light_blob_i.rect.angle - 90;
        float angle_j = light_blob_j.rect.size.width > light_blob_j.rect.size.height ? light_blob_j.rect.angle :
                        light_blob_j.rect.angle - 90;
        float angle = (angle_i + angle_j) / 2.0;
        if (abs(angle_i - angle_j) > 90) {
            angle += 90.0;
        }
        return (-120.0 < angle && angle < -60.0) || (60.0 < angle && angle < 120.0);
    }
    //判断是否为一对灯条
    static bool isCoupleLight(const LightBlob &light_blob_i, const LightBlob &light_blob_j, uint8_t enemy_color) 
    {
        return CuoWeiDuJudge(light_blob_i, light_blob_j) &&
            lengthRatioJudge(light_blob_i, light_blob_j) &&
            lengthJudge(light_blob_i, light_blob_j) &&
            angelJudge(light_blob_i, light_blob_j);
        /*
        return light_blob_i.blob_color == enemy_color &&
           light_blob_j.blob_color == enemy_color &&
           lengthRatioJudge(light_blob_i, light_blob_j) &&
           lengthJudge(light_blob_i, light_blob_j) &&
           //           heightJudge(light_blob_i, light_blob_j) &&
           angelJudge(light_blob_i, light_blob_j) &&
           boxAngleJudge(light_blob_i, light_blob_j) &&
           CuoWeiDuJudge(light_blob_i, light_blob_j);
        */
    }
    /**
     * @func:匹配装甲板,外部调用只需调用该函数即可
     * @para:src:一帧图像,light_bolbs:灯条存储vector，armor_boxes:装甲板存储vector，匹配到的装甲板存储到此处
     * 
     **/
 bool AutoAiming::matchArmorBoxes(const cv::Mat &src, const LightBlobs &light_blobs, ArmorBoxes &armor_boxes){
        armor_boxes.clear();
        for (int i = 0; i < light_blobs.size() - 1; ++i) {
            for (int j = i + 1; j < light_blobs.size(); ++j) {
                if (!isCoupleLight(light_blobs.at(i), light_blobs.at(j), BLOB_RED)) {
                    //cout<<"Not pair blobs"<<endl;
                    continue;
                }
                //cout<<"pair blobs"<<endl;
                Rect2d rect_left = light_blobs.at(static_cast<unsigned long>(i)).rect.boundingRect();
                Rect2d rect_right = light_blobs.at(static_cast<unsigned long>(j)).rect.boundingRect();
                double min_x, min_y, max_x, max_y;
                min_x = fmin(rect_left.x, rect_right.x) - 4;
                max_x = fmax(rect_left.x + rect_left.width, rect_right.x + rect_right.width) + 4;
                min_y = fmin(rect_left.y, rect_right.y) - 0.5 * (rect_left.height + rect_right.height) / 2.0;
                max_y = fmax(rect_left.y + rect_left.height, rect_right.y + rect_right.height) +
                        0.5 * (rect_left.height + rect_right.height) / 2.0;
                
                if (min_x < 0 || max_x > src.cols || min_y < 0 || max_y > src.rows) {
                    cout<<"Wrong Size Box"<<endl;
                    continue;
                }
                //if ((max_y + min_y) / 2 < 120) continue;
                if ((max_x - min_x) / (max_y - min_y) < 0.8) continue;
                
                LightBlobs pair_blobs = {light_blobs.at(i), light_blobs.at(j)};
                armor_boxes.emplace_back(
                        cv::Rect2d(min_x, min_y, max_x - min_x, max_y - min_y),
                        pair_blobs,
                        BLOB_RED
                );
            }
        }
    //cout<<"armor boxes number"<<armor_boxes.size()<<endl;
    return !armor_boxes.empty();
    }
    
    //在src上将装甲板框出
    void AutoAiming::showArmorBoxes(std::string windows_name, const cv::Mat &g_srcImage, const ArmorBoxes &armor_boxes) {
        
        static Mat image2show4boxes;
        if (g_srcImage.type() == CV_8UC1) {// 黑白图像
            cvtColor(g_srcImage, image2show4boxes, COLOR_GRAY2RGB);
        } else if (g_srcImage.type() == CV_8UC3) { //RGB 彩色
            image2show4boxes = g_srcImage.clone();
        }

        for (auto &box:armor_boxes) {
            if(box.box_color == BLOB_BLUE) {
                rectangle(image2show4boxes, box.rect, Scalar(0, 255, 0), 1);
            }else if(box.box_color == BLOB_RED){
                rectangle(image2show4boxes, box.rect, Scalar(0, 255, 0), 1);
            }
        }
        namedWindow(windows_name,0);
        resizeWindow(windows_name,600,400);
        imshow(windows_name, image2show4boxes);
        waitKey(1);
    }
void AutoAiming::showArmorBox(std::string windows_name, const cv::Mat &g_srcImage, const cv::Rect2d &armor_box) {
        
        static Mat image2show4box;
        if (g_srcImage.type() == CV_8UC1) {// 黑白图像
            cvtColor(g_srcImage, image2show4box, COLOR_GRAY2RGB);
        } else if (g_srcImage.type() == CV_8UC3) { //RGB 彩色
            image2show4box = g_srcImage.clone();
        }


        rectangle(image2show4box, armor_box, Scalar(0, 255, 0), 1);
        namedWindow(windows_name,0);
        resizeWindow(windows_name,600,400);
        imshow(windows_name, image2show4box);
        waitKey(1);
    }
