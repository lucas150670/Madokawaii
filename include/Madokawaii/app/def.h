// ============================================================
// Project: Madokawaii
// Route: include/Madokawaii/app/def.h
// Usage: Store consistent data (constants, enums, type definitions)
// ============================================================

#ifndef MADOKAWAII_DEF_H
#define MADOKAWAII_DEF_H

#include <string>
#include <vector>

#ifdef M_PI
#undef M_PI
#endif
#define M_PI 3.14159265358979323846


namespace Madokawaii::App {
    enum NoteType {
        tap = 1,
        drag = 2,
        flick = 4,
        hold = 3
    };

    enum NoteState {
        finished, // 结束渲染
        holding, // hold判定中
        invisible_or_appeared
    };

    /**
     * @brief Chart Structure
     */
    struct chart {
        int formatVersion{};
        double offset{};
        int judgelineCount{};

        // int numOfNotes;
        struct judgeline {
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
                int type;
                double time;
                double positionX, holdTime, speed, floorPosition;
                // real-time calculated values
                double realTime, positionY, coordinateX, coordinateY, realHoldTime;
                double rotateAngle;
                bool isNoteBelow, isMultipleNote;
                int state;
                int parent_line_id;
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
        };

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
