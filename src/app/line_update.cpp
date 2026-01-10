//
// Created by madoka on 2025/9/19.
//

#include "Madokawaii/app/chart.h"
#include "Madokawaii/app/line_operation.h"
#include <Madokawaii/platform/log.h>

#include "Madokawaii/app/note_hit.h"
#include "Madokawaii/app/note_operation.h"

void UpdateJudgeline(Madokawaii::App::chart::judgeline& judgeline, double thisFrameTime, int screenWidth, int screenHeight, std::vector<Madokawaii::App::chart::judgeline::note*>& noteRenderList) {
	auto calcEventRealTime = [&judgeline](const double beatTime) {
		return Madokawaii::App::Chart::CalcRealTime(judgeline.bpm, beatTime);
		};

	for (; calcEventRealTime(judgeline.info.disappearEventPointer->endTime) < thisFrameTime; ++judgeline.info.disappearEventPointer) {
		judgeline.info.opacity = judgeline.info.disappearEventPointer->end;
	}
	for (; calcEventRealTime(judgeline.info.moveEventPointer->endTime) < thisFrameTime; ++judgeline.info.moveEventPointer) {
		judgeline.info.posX = judgeline.info.moveEventPointer->end;
		judgeline.info.posY = judgeline.info.moveEventPointer->end2;
	}
	for (; calcEventRealTime(judgeline.info.rotateEventPointer->endTime) < thisFrameTime; ++judgeline.info.rotateEventPointer) {
		judgeline.info.rotateAngle = judgeline.info.rotateEventPointer->end;
	}
	for (; calcEventRealTime(judgeline.info.speedEventPointer->endTime) < thisFrameTime; ++judgeline.info.speedEventPointer) {
		judgeline.info.positionY = judgeline.info.speedEventPointer->floorPosition;
	}

	judgeline.info.opacity = Madokawaii::App::Chart::CalcEventProgress1Param(*judgeline.info.disappearEventPointer,
		Madokawaii::App::Chart::CalcBeatTime(judgeline.bpm, thisFrameTime));
	auto point = Madokawaii::App::Chart::CalcEventProgress2Params(*judgeline.info.moveEventPointer,
		Madokawaii::App::Chart::CalcBeatTime(judgeline.bpm, thisFrameTime));
	judgeline.info.posX = std::get<0>(point);
	judgeline.info.posY = std::get<1>(point);
	judgeline.info.rotateAngle = Madokawaii::App::Chart::CalcEventProgress1Param(*judgeline.info.rotateEventPointer,
		Madokawaii::App::Chart::CalcBeatTime(judgeline.bpm, thisFrameTime),
		[](double angle) {
			while (angle < 0) angle += 360;
			while (angle > 360) angle -= 360;
			return angle;
		});

	// note update
	judgeline.info.positionY = judgeline.info.speedEventPointer->floorPosition + (thisFrameTime - calcEventRealTime(judgeline.info.speedEventPointer->startTime)) * judgeline.info.speedEventPointer->value;
	const double judgelineScreenX = judgeline.info.posX * screenWidth;
	const double judgelineScreenY = judgeline.info.posY * screenHeight;

	auto processNote = [&, thisFrameTime](Madokawaii::App::chart::judgeline::note &note) {
		note.rotateAngle = judgeline.info.rotateAngle;
		if (fabs(note.rotateAngle - 360.0) < 1e-6)
			note.rotateAngle = 0.0;

		// calc positionY
		if (note.type != Madokawaii::App::NoteType::hold)
			note.positionY = note.speed * (note.floorPosition - judgeline.info.positionY);
		else
			note.positionY = note.floorPosition - judgeline.info.positionY;

		note.realHoldTime = Madokawaii::App::Chart::CalcRealTime(judgeline.bpm, note.holdTime);

		// unify note position calculation
		const double note_rotate_angle_rad = note.rotateAngle * M_PI / 180.0;
		const double posX = note.positionX * screenWidth * 0.05625;
		double distance = note.positionY;
		if (note.isNoteBelow) {
			distance = -distance;
		}

		const double diffX = cos(note_rotate_angle_rad) * posX
		                     - sin(note_rotate_angle_rad) * distance * screenHeight * 0.6;
		const double diffY = cos(note_rotate_angle_rad) * distance * screenHeight * 0.6
		                     + sin(note_rotate_angle_rad) * posX;

		note.coordinateX = judgelineScreenX + diffX;
		note.coordinateY = screenHeight - (judgelineScreenY + diffY);

		if (note.realTime < thisFrameTime && note.state == Madokawaii::App::NoteState::invisible_or_appeared) {
			note.coordinateX = judgelineScreenX + cos(note_rotate_angle_rad) * posX;
			note.coordinateY = screenHeight - (judgelineScreenY + sin(note_rotate_angle_rad) * posX);

			RegisterNoteHitSfx(note.type);
			RegisterNoteHitFx(thisFrameTime, note.coordinateX, note.coordinateY);

			if (note.type == Madokawaii::App::NoteType::hold) {
				AddHoldNoteClickingRender(note);
				note.state = Madokawaii::App::NoteState::holding;
			} else {
				note.state = Madokawaii::App::NoteState::finished;
			}
		}

		switch (note.state) {
			case Madokawaii::App::NoteState::holding:
			case Madokawaii::App::NoteState::finished:
				if (note.isNoteBelow)
					++judgeline.info.notesBelowIndex;
				else
					++judgeline.info.notesAboveIndex;
				return false;

			case Madokawaii::App::NoteState::invisible_or_appeared:
				if (note.type == Madokawaii::App::NoteType::hold ||
				    Madokawaii::App::Chart::IsNoteInScreen(note.coordinateX, note.coordinateY, screenWidth,
				                                           screenHeight)) {
					noteRenderList.push_back(&note);
					return false;
				}
				return true;

			default:
				return false;
		}
	};

	for (size_t index = judgeline.info.notesAboveIndex; index < judgeline.notesAbove.size(); ++index) {
		if (processNote(judgeline.notesAbove[index])) break;
	}
	for (size_t index = judgeline.info.notesBelowIndex; index < judgeline.notesBelow.size(); ++index) {
		if (processNote(judgeline.notesBelow[index])) break;
	}
}