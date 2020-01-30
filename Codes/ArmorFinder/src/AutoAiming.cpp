#include "ArmorFinder.h"

using namespace cv;
using namespace std;

bool ArmorFinder::stateTrackingTarget(cv::Mat &src) {
    //位置目标
    auto pos = target_box.rect;
    if(!tracker->update(src, pos)){ // 使用KCFTracker进行追踪
        //追踪失败
        target_box = ArmorBox();
        LOGW("Track fail!");
        return false;
    }
    //追踪出界
    if((pos & cv::Rect2d(0, 0, 640, 480)) != pos){
        target_box = ArmorBox();
        LOGW("Track out range!");
        return false;
    }

    // 获取相较于追踪区域两倍长款的区域，用于重新搜索，获取灯条信息
    cv::Rect2d bigger_rect;
    bigger_rect.x = pos.x - pos.width / 2.0;
    bigger_rect.y = pos.y - pos.height / 2.0;
    bigger_rect.height = pos.height * 2;
    bigger_rect.width  = pos.width * 2;
    bigger_rect &= cv::Rect2d(0, 0, 640, 480);
    cv::Mat roi = src(bigger_rect).clone();
    /*
    ArmorBox box;
    // 在区域内重新搜索。
    if(findArmorBox(roi, box)) { // 如果成功获取目标，则利用搜索区域重新更新追踪器
        target_box = box;
        target_box.rect.x += bigger_rect.x; //　添加roi偏移量
        target_box.rect.y += bigger_rect.y;
        for(auto &blob : target_box.light_blobs){
            blob.rect.center.x += bigger_rect.x;
            blob.rect.center.y += bigger_rect.y;
        }
        tracker = TrackerToUse::create();
        tracker->init(src, target_box.rect);
    }else{    // 如果没有成功搜索目标，则使用判断是否跟丢。
        roi = src(pos).clone();
        if(classifier){ // 分类器可用，使用分类器判断。
            cv::resize(roi, roi, cv::Size(48, 36));
            if(classifier(roi) == 0){
                target_box = ArmorBox();
                LOGW("Track classify fail range!");
                return false;
            }
        }else{ //　分类器不可用，使用常规方法判断
            cv::Mat roi_gray;
            cv::cvtColor(roi, roi_gray, CV_RGB2GRAY);
            cv::threshold(roi_gray, roi_gray, 180, 255, cv::THRESH_BINARY);
            contour_area = cv::countNonZero(roi_gray);
            if(abs(cv::countNonZero(roi_gray) - contour_area) > contour_area * 0.3){
                target_box = ArmorBox();
                return false;
            }
        }
        target_box.rect = pos;
        target_box.light_blobs.clear();
    }*/
    return true;
}

void AutoAiming::run(cv::Mat &g_srcImage,cv::Mat &g_processImage)
{
    switch(state)
    {
        case SEARCHING_STATE:
            if( lightBox(g_processImage) )
            {
                //调高曝光
                if(true)        //这地方到时候加入分类器
                {
                    tracker = TrackerKCF::create();
                    tracker->init(g_srcImage, target_box.rect);
                    //状态改为追踪状态
                    state = TRACKING_STATE;
                    //追踪帧数
                    tracking_cnt = 0;
                }
            }
            break;
        case TRACKING_STATE:
            if (!stateTrackingTarget(g_srcImage) || ++tracking_cnt > 100) {    // 最多追踪100帧图像
                state = SEARCHING_STATE;
            }
            break;
    }
}