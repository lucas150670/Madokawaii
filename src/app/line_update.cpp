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
		return Madokawaii::App::Chart::CalcRealTime(judgeline.bpm, static_cast<int>(beatTime));
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

	auto processNote = [&, thisFrameTime](Madokawaii::App::chart::judgeline::note& note) {
		if (note.realTime < thisFrameTime) {
			RegisterNoteHitSfx(note.type);
			RegisterNoteHitFx(thisFrameTime, note.coordinateX, note.coordinateY);
			if (note.type == Madokawaii::App::NoteType::hold) {
				AddHoldNoteClickingRender(note);
				note.state = Madokawaii::App::NoteState::holding;
			}
			else
				note.state = Madokawaii::App::NoteState::finished;
			auto diff = thisFrameTime - note.realTime;
			if (fabs(diff) > 1e-3f) {
				Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_WARNING,
					"NOTE: abnormal time event, realTime=%f, thisTime=%f, diff=%f", note.realTime, thisFrameTime, diff);
			}
		}
		if (note.type != Madokawaii::App::NoteType::hold)
			note.positionY = note.speed * (note.floorPosition - judgeline.info.positionY);
		else
			note.positionY = note.floorPosition - judgeline.info.positionY;
		note.rotateAngle = judgeline.info.rotateAngle;
		// realholdtime = speed * (holdTime -> real)
		note.realHoldTime = Madokawaii::App::Chart::CalcRealTime(judgeline.bpm, static_cast<int>(note.holdTime));
		if (fabs(note.rotateAngle - 360.0) < 1e-6)
			note.rotateAngle = 0.0f;
		auto note_rotate_angle_rad =  note.rotateAngle * M_PI / 180.0;

		switch (note.state) {
		case Madokawaii::App::NoteState::holding:
		case Madokawaii::App::NoteState::finished:
			if (note.isNoteBelow) {
				++judgeline.info.notesBelowIndex;
				// Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "INFO: noteBelowIndex = %d holding/disappear, incrasing index", judgeline.info.notesBelowIndex - 1);
			}
			else {
				++judgeline.info.notesAboveIndex;
				// Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "INFO: noteAboveIndex = %d holding/disappear, incrasing index", judgeline.info.notesAboveIndex - 1);
			}
			break;
		case Madokawaii::App::NoteState::invisible_or_appeared:
		{
			const double posX = note.positionX * screenWidth * 0.05625;

			double distance = note.positionY;
			if (note.isNoteBelow) {
				distance = -distance;
			}

			const double diffX = cos(note_rotate_angle_rad) * posX - sin(note_rotate_angle_rad) * distance * screenHeight * 0.6;
			const double diffY = cos(note_rotate_angle_rad) * distance * screenHeight * 0.6 + sin(note_rotate_angle_rad) * posX;

			note.coordinateX = judgelineScreenX + diffX;
			const double centralY = judgelineScreenY + diffY;
			note.coordinateY = screenHeight - centralY;

			// TODO: Implement Hold Note Collider Detection
			if (note.type == Madokawaii::App::NoteType::hold || Madokawaii::App::Chart::IsNoteInScreen(note.coordinateX, note.coordinateY, screenWidth, screenHeight)) {
				/*
				if (fabs(note.rotateAngle) > 1e-6)
					Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO,
						"NOTE: Note appeared at rotate angle %f (realTime %f), position (%f, %f)",
				note.rotateAngle, note.realTime, note.coordinateX, note.coordinateY);*/
				noteRenderList.push_back(&note);
			}
			else
				return true;
		}
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