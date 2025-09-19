// ============================================================
// Project: Madokawaii
// Route: include/Madokawaii/app/def.h
// Usage: Store consistent data (constants, enums, type definitions)
// ============================================================

#ifndef MADOKAWAII_DEF_H
#define MADOKAWAII_DEF_H

#include <string>
#include <vector>


namespace Madokawaii::Defs {
    enum NoteType {
        tap = 1,
        drag = 4,
        flick = 3,
        hold = 2
    };

    enum NoteState {
        finished, // 结束渲染
        holding, // hold判定中
        appeared, // 出现在视距中
        invisible // 不可见
    };

    /**
     * @brief Chart Structure
     */
    struct chart {
        int formatVersion{};
        double offset{};
        int judgelineCount{};

        // int numOfNotes;
        typedef struct {
            int id{};
            // int numOfNotes;
            // int numOfNotesAbove;
            // int numOfNotesBelow;
            double bpm{};

            typedef struct {
                double startTime, endTime;
                double start, end, start2, end2;
                double value;
                // real-time calculated values
                double floorPosition;
            } event_base;

            std::vector<event_base> speedEvents;

            typedef struct {
                int type, time;
                double positionX, holdTime, speed, floorPosition;
                // real-time calculated values
                double realTime, positionY, coordinateX, coordinateY;
                double rotateAngle;
                bool isNoteBelow, isMultipleNote;
                int state;
            } note;

            struct {
                double opacity{}; // 透明度
                double posX{}, posY{}; // 横坐标，纵坐标
                double rotateAngle{}; // 旋转角(DEG)
                double speed{}; // note下落速度
                double positionY{}; // speedEvents计算得到 ，note.floorPosition-positionY为对线距离
                // 事件指针
                std::vector<event_base>::const_iterator disappearEventPointer, moveEventPointer, rotateEventPointer,
                        speedEventPointer;
                // note指针
                size_t notesAboveIndex{}, notesBelowIndex{};
            } info;

            std::vector<note> notesAbove, notesBelow;
            std::vector<event_base> judgelineDisappearedEvents, judgelineMoveEvents, judgelineRotateEvents;
        } judgeline;

        std::vector<judgeline> judgelines;
        // debug info
        int speedEventsCount{}, disappearEventCount{}, moveEventCount{}, rotateEventCount{};
        int numOfNotes{};
    };

    /**
     * @brief 资源包数据结构，兼容prpr/phira
     */
    struct respack {
        std::string name, author;
        int hitFx[2], holdAtlas[2], holdAtlasMH[2];
        // adapted for zip-based respack
        static const std::string clickOggRoute,
                clickRoute,
                clickMHRoute,
                dragOggRoute,
                dragRoute,
                dragMHRoute,
                endingMp3Route,
                flickOggRoute,
                flickRoute,
                flickMHRoute,
                hitFxRoute,
                holdRoute,
                holdMHRoute;
    };
}


#endif //MADOKAWAII_DEF_H
