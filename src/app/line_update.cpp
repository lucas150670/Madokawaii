//
// Created by madoka on 2025/9/19.
//

#include <cmath>
#include "Madokawaii/app/chart.h"
#include "Madokawaii/app/line_operation.h"

void UpdateJudgeline(Madokawaii::Defs::chart::judgeline& judgeline, double thisFrameTime, int screenWidth, int screenHeight, std::vector<Madokawaii::Defs::chart::judgeline::note *>& noteRenderList) {
    auto calcEventRealTime = [&judgeline](const double beatTime) {
        return Madokawaii::Defs::Chart::CalcRealTime(judgeline.bpm, static_cast<int>(beatTime));
    };

    for (; !(calcEventRealTime(judgeline.info.disappearEventPointer->endTime) > thisFrameTime); ++judgeline.info.disappearEventPointer) {
        judgeline.info.opacity = judgeline.info.disappearEventPointer->end;
    }
    for (; !(calcEventRealTime(judgeline.info.moveEventPointer->endTime) > thisFrameTime); ++judgeline.info.moveEventPointer) {
        judgeline.info.posX = judgeline.info.moveEventPointer->end;
        judgeline.info.posY = judgeline.info.moveEventPointer->end2;
    }
    for (; !(calcEventRealTime(judgeline.info.rotateEventPointer->endTime) > thisFrameTime); ++judgeline.info.rotateEventPointer) {
        judgeline.info.rotateAngle = judgeline.info.rotateEventPointer->end;
    }
    for (; !(calcEventRealTime(judgeline.info.speedEventPointer->endTime) > thisFrameTime); ++judgeline.info.speedEventPointer) {
        judgeline.info.positionY = judgeline.info.speedEventPointer->floorPosition;
    }

    judgeline.info.opacity = Madokawaii::Defs::Chart::CalcEventProgress1Param(*judgeline.info.disappearEventPointer,
                                                                              Madokawaii::Defs::Chart::CalcBeatTime(judgeline.bpm, thisFrameTime));
    auto point = Madokawaii::Defs::Chart::CalcEventProgress2Params(*judgeline.info.moveEventPointer,
                                                                   Madokawaii::Defs::Chart::CalcBeatTime(judgeline.bpm, thisFrameTime));
    judgeline.info.posX = std::get<0>(point);
    judgeline.info.posY = std::get<1>(point);
    judgeline.info.rotateAngle = Madokawaii::Defs::Chart::CalcEventProgress1Param(*judgeline.info.rotateEventPointer,
                                                                                  Madokawaii::Defs::Chart::CalcBeatTime(judgeline.bpm, thisFrameTime),
                                                                                  [](double angle) {
        while (angle < 0) angle += 360;
        while (angle > 360) angle -= 360;
        return angle;
    });

    // note update
    judgeline.info.positionY = judgeline.info.speedEventPointer->floorPosition + (thisFrameTime - calcEventRealTime(judgeline.info.speedEventPointer->startTime)) * judgeline.info.speedEventPointer->value;

    auto processNote = [&, thisFrameTime](Madokawaii::Defs::chart::judgeline::note& note) {
        if (note.realTime < thisFrameTime) {
            note.state = Madokawaii::Defs::NoteState::finished;
        }
        note.positionY = judgeline.info.positionY - note.floorPosition;
        note.rotateAngle = judgeline.info.rotateAngle / 180.0 * M_PI;

        switch (note.state) {
            case Madokawaii::Defs::NoteState::finished:
            case Madokawaii::Defs::NoteState::holding:
                if (note.isNoteBelow)
                    ++judgeline.info.notesBelowIndex;
                else
                    ++judgeline.info.notesAboveIndex;
                break;
            case Madokawaii::Defs::NoteState::invisible:
            case Madokawaii::Defs::NoteState::appeared:
            {
                const double posY = note.isNoteBelow ? -note.positionY : note.positionY;
                const double diffX = cos(note.rotateAngle) * note.positionX - sin(note.rotateAngle) * posY;
                const double diffY = sin(note.rotateAngle) * note.positionX + cos(note.rotateAngle) * posY;
                note.coordinateX = judgeline.info.posX + diffX;
                note.coordinateY = judgeline.info.posY + diffY;
                if (Madokawaii::Defs::Chart::IsNoteInScreen(note.coordinateX, note.coordinateY, screenWidth, screenHeight)) {
                    note.state = Madokawaii::Defs::NoteState::appeared;
                }
                if (note.state == Madokawaii::Defs::NoteState::appeared)
                    noteRenderList.push_back(&note);
                else
                    return true;
            }
                break;
            default:
                break;
        }
        return false;
    };

    for (size_t index = judgeline.info.notesAboveIndex; index < judgeline.notesAbove.size(); ++index) {
        if (processNote(judgeline.notesAbove[index])) break;
    }
    for (size_t index = judgeline.info.notesBelowIndex; index < judgeline.notesBelow.size(); ++index) {
        if (processNote(judgeline.notesBelow[index])) break;
    }
}
