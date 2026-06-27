//
// Created by madoka on 2025/9/19.
//

#include "Madokawaii/app/chart.hpp"
#include "Madokawaii/app/common.hpp"
#include "Madokawaii/app/coordinate.hpp"
#include "Madokawaii/app/line_operation.hpp"
#include <Madokawaii/platform/log.hpp>

#include "Madokawaii/app/note_hit.hpp"
#include "Madokawaii/app/note_operation.hpp"

namespace Madokawaii::App::Line {

void UpdateJudgeline(AppContext& context, Madokawaii::App::chart::judgeline& judgeline, double thisFrameTime, std::vector<Madokawaii::App::chart::judgeline::note*>& noteRenderList, int* playedNoteCount) {
	auto calcEventRealTime = [&judgeline](const double beatTime) {
		return Madokawaii::App::Chart::CalcRealTime(judgeline.bpm, beatTime);
		};

	if (!judgeline.judgelineDisappearedEvents.empty()) {
		for (; judgeline.info.disappearEventPointer != judgeline.judgelineDisappearedEvents.end()
			&& calcEventRealTime(judgeline.info.disappearEventPointer->endTime) < thisFrameTime; ++judgeline.info.disappearEventPointer) {
			judgeline.info.opacity = judgeline.info.disappearEventPointer->end;
		}
		if (judgeline.info.disappearEventPointer != judgeline.judgelineDisappearedEvents.end())
			judgeline.info.opacity = Madokawaii::App::Chart::CalcEventProgress1Param(*judgeline.info.disappearEventPointer,
				Madokawaii::App::Chart::CalcBeatTime(judgeline.bpm, thisFrameTime));
	}
	if (!judgeline.judgelineMoveEvents.empty()) {
		for (; judgeline.info.moveEventPointer != judgeline.judgelineMoveEvents.end()
				&& calcEventRealTime(judgeline.info.moveEventPointer->endTime) < thisFrameTime; ++judgeline.info.moveEventPointer) {
			judgeline.info.posX = judgeline.info.moveEventPointer->end;
			judgeline.info.posY = judgeline.info.moveEventPointer->end2;
		}
		if (judgeline.info.moveEventPointer != judgeline.judgelineMoveEvents.end()) {
			auto point = Madokawaii::App::Chart::CalcEventProgress2Params(*judgeline.info.moveEventPointer,
				Madokawaii::App::Chart::CalcBeatTime(judgeline.bpm, thisFrameTime));
			judgeline.info.posX = std::get<0>(point);
			judgeline.info.posY = std::get<1>(point);
		}
	}
	if (!judgeline.judgelineRotateEvents.empty()) {
		for (; judgeline.info.rotateEventPointer != judgeline.judgelineRotateEvents.end()
				&& calcEventRealTime(judgeline.info.rotateEventPointer->endTime) < thisFrameTime; ++judgeline.info.rotateEventPointer) {
			judgeline.info.rotateAngle = judgeline.info.rotateEventPointer->end;
		}
		if (judgeline.info.rotateEventPointer != judgeline.judgelineRotateEvents.end()) {
			judgeline.info.rotateAngle = Madokawaii::App::Chart::CalcEventProgress1Param(*judgeline.info.rotateEventPointer,
				Madokawaii::App::Chart::CalcBeatTime(judgeline.bpm, thisFrameTime),
				[](double angle) {
					while (angle < 0) angle += 360;
					while (angle > 360) angle -= 360;
					return angle;
				});
		}
	}
	if (!judgeline.speedEvents.empty()) {
		for (; judgeline.info.speedEventPointer != judgeline.speedEvents.end()
				&& calcEventRealTime(judgeline.info.speedEventPointer->endTime) < thisFrameTime; ++judgeline.info.speedEventPointer) {
			judgeline.info.positionY = judgeline.info.speedEventPointer->floorPosition;
		}
		if (judgeline.info.speedEventPointer != judgeline.speedEvents.end()) {
			judgeline.info.positionY = judgeline.info.speedEventPointer->floorPosition + (thisFrameTime - calcEventRealTime(judgeline.info.speedEventPointer->startTime)) * judgeline.info.speedEventPointer->value;
		}
	}

	// note update
	const auto screenWidth = context.display.screenWidth;
	const auto screenHeight = context.display.screenHeight;
	const auto viewport = Madokawaii::App::Coordinate::MakeScreenViewport(screenWidth, screenHeight);
	const Madokawaii::App::Coordinate::NormalizedPoint judgelinePosition{
		judgeline.info.posX,
		judgeline.info.posY
	};

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

		// Keep calculated note centers in normalized chart coordinates.
		const double posX = note.positionX * 0.05625;
		double distance = note.positionY;
		if (note.isNoteBelow) {
			distance = -distance;
		}
		distance *= 0.6;

		const auto noteOffset = Madokawaii::App::Coordinate::RotateNormalizedVector(
			{posX, distance},
			note.rotateAngle,
			viewport);

		note.coordinateX = judgelinePosition.x + noteOffset.x;
		note.coordinateY = judgelinePosition.y + noteOffset.y;

		if (note.realTime < thisFrameTime && note.state == Madokawaii::App::NoteState::invisible_or_appeared) {
			const auto hitOffset = Madokawaii::App::Coordinate::RotateNormalizedVector(
				{posX, 0.0},
				note.rotateAngle,
				viewport);
			note.coordinateX = judgelinePosition.x + hitOffset.x;
			note.coordinateY = judgelinePosition.y + hitOffset.y;

			NoteHit::RegisterSfx(context, note.type);
			NoteHit::RegisterFx(context, thisFrameTime, note.coordinateX, note.coordinateY);

			if (note.type == Madokawaii::App::NoteType::hold) {
				NoteRenderer::AddHoldNoteClickingRender(context, note);
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
				    Madokawaii::App::Chart::IsNoteInViewport(note.coordinateX, note.coordinateY, screenWidth,
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
	if (playedNoteCount)
		*playedNoteCount = static_cast<int>(judgeline.info.notesAboveIndex + judgeline.info.notesBelowIndex);
}

} // namespace Madokawaii::App::Line
