
#include <algorithm>
#include <ctime>
#include <cfloat>
#include <format>
#include <string>
#include <tuple>
#include <vector>
#include <cmath>

#include "Madokawaii/app/def.h"
#include "Madokawaii/app/chart.h"
#include "Madokawaii/platform/audio.h"
#include "Madokawaii/platform/log.h"
#include "Madokawaii/platform/core.h"
#include "Madokawaii/platform/graphics.h"

int main() {
    constexpr int screenWidth = 1920;
    constexpr int screenHeight = 1080;

    Madokawaii::Platform::Audio::InitAudioDevice();
    
    // load music
    clock_t begin = clock();
    if (!Madokawaii::Platform::Core::FileExists(musicPath.c_str())) {
        Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_ERROR, "MAIN: Music file does not exist!");
        return -1;
    }
    auto music = Madokawaii::Platform::Audio::LoadMusicStream(musicPath.c_str());
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "MAIN: Music stream loaded");
    
    // load chart using chart module
    auto mainChart = Madokawaii::Defs::Chart::LoadChartFromFile(chartPath.c_str());
    if (!Madokawaii::Defs::Chart::IsValidChart(mainChart)) {
        Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_ERROR, "MAIN: Failed to load chart!");
        return -1;
    }

    clock_t end = clock();
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "MAIN: Chart Initialization Successful!");
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "    > Format Version:         %d", mainChart.formatVersion);
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "    > Number of notes:        %d", mainChart.numOfNotes);
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "    > Number of judgelines:   %d", mainChart.judgelineCount);
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "    > SpeedEvents Count:      %d", mainChart.speedEventsCount);
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "    > DisappearEvents Count:  %d", mainChart.disappearEventCount);
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "    > MoveEvents Count:       %d", mainChart.moveEventCount);
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "    > RotateEvents Count:     %d", mainChart.rotateEventCount);
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "MAIN: Elapsed time: %lf s", (end - begin) * 1.0 / CLOCKS_PER_SEC);

    // initialize judgelines, set start value
    Madokawaii::Defs::Chart::InitializeJudgelines(mainChart);

    Madokawaii::Platform::Core::InitWindow(screenWidth, screenHeight, "Madokawaii");

    auto glVersion = Madokawaii::Platform::Graphics::GetImplementationInfo(),
         glRenderer = Madokawaii::Platform::Graphics::GetImplementer();

    music.looping = false;
    auto musicLength = GetMusicTimeLength(music);
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "MAIN: Music Length: %f", musicLength);
    PlayMusicStream(music);

    while (!Madokawaii::Platform::Core::WindowShouldClose()) {
        Madokawaii::Platform::Audio::UpdateMusicStream(music);
        Madokawaii::Platform::Graphics::BeginDrawing();
        Madokawaii::Platform::Graphics::ClearBackground(Madokawaii::Platform::Graphics::BLACK);

        auto thisFrameTime = Madokawaii::Platform::Audio::GetMusicTimePlayed(music);
        if (!Madokawaii::Platform::Audio::IsMusicStreamPlaying(music)) {
            Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "MAIN: Music playback end");
            Madokawaii::Platform::Graphics::EndDrawing();
            break;
        }
        auto noteRenderList = std::vector<Madokawaii::Defs::chart::judgeline::note *>();

        // update judgeline & render
        for (auto &judgeline: mainChart.judgelines) {
            // for debug breakpoint use
            if (judgeline.id == 1) {
                judgeline.id = 1;
            }
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

            // line render
            auto c = Madokawaii::Platform::Graphics::RAYWHITE;
            c.a = static_cast<unsigned char>(judgeline.info.opacity * 255);
            auto screenX = static_cast<float>(judgeline.info.posX * screenWidth),
                    screenY = static_cast<float>((1 - judgeline.info.posY) * screenHeight),
                aspectRatio = screenWidth * 1.0f / screenHeight;
            if (fabs(judgeline.info.rotateAngle) < 1e-6 || fabs(judgeline.info.rotateAngle - 180.0f) < 1e-6) {
                DrawLineEx(Madokawaii::Platform::Graphics::Vector2{screenX - 5000, screenY}, Madokawaii::Platform::Graphics::Vector2{screenX + 5000, screenY}, 10.0f, c);
            } else if (fabs(judgeline.info.rotateAngle - 90.0f) < 1e-6 || fabs(judgeline.info.rotateAngle - 270.0f) < 1e-6) {
                DrawLineEx(Madokawaii::Platform::Graphics::Vector2{screenX, screenY - 5000}, Madokawaii::Platform::Graphics::Vector2{screenX, screenY + 5000}, 10.0f, c);
            } else {
                float k, kx0, y0;
                k = static_cast<float>(tan(judgeline.info.rotateAngle / 180.0 * M_PI) * aspectRatio);
                kx0 = static_cast<float>(k * judgeline.info.posX),
                y0 = static_cast<float>(judgeline.info.posY);
                Madokawaii::Platform::Graphics::Vector2 p1{}, p2{};
                p1 = {-10 * screenWidth, screenHeight - (-10 * k - kx0 + y0) * screenHeight};
                p2 = {10 * screenWidth, screenHeight - (10 * k - kx0 + y0) * screenHeight};
                DrawLineEx(p1, p2, 10.0f, c);
            }
            
            // 画线ID
            auto idStr = std::format("{}", judgeline.id);
            Madokawaii::Platform::Graphics::DrawText(idStr.c_str(), static_cast<int>(screenX), static_cast<int>(screenY), 30, Madokawaii::Platform::Graphics::RED);
            
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
        
        // 遍历note列表，对多押note打标
        for (auto notePtrIter = noteRenderList.begin(); notePtrIter != noteRenderList.end(); ++notePtrIter) {
            for (auto notePtrIter1 = notePtrIter; notePtrIter1 != noteRenderList.end(); ++notePtrIter1) {
                if (fabs((*notePtrIter)->realTime - (*notePtrIter)->realTime) < 1e-6) {
                    (*notePtrIter1)->isMultipleNote = (*notePtrIter)->isMultipleNote = true;
                } else if ((*notePtrIter)->realTime < (*notePtrIter)->realTime) {
                    break;
                }
            }
        }
        
        // 渲染note
        for (auto notePtr: noteRenderList) {
            // TODO: 实现note渲染
        }
        
        auto dbgStr = std::format("OpenGL Renderer: {}", glRenderer);
        DrawText(dbgStr.c_str(), 190, 170, 20, Madokawaii::Platform::Graphics::LIGHTGRAY);
        dbgStr = std::format("OpenGL Version: {}", glVersion);
        DrawText(dbgStr.c_str(), 190, 200, 20, Madokawaii::Platform::Graphics::LIGHTGRAY);
        dbgStr = std::format("FPS: {}, FrameTime: {}s", Madokawaii::Platform::Graphics::GetFPS(), Madokawaii::Platform::Graphics::GetFrameTime());
        DrawText(dbgStr.c_str(), 190, 230, 20, Madokawaii::Platform::Graphics::LIGHTGRAY);

        Madokawaii::Platform::Graphics::EndDrawing();
    }

    Madokawaii::Platform::Core::CloseWindow();
    if (Madokawaii::Platform::Audio::IsMusicStreamPlaying(music))
        Madokawaii::Platform::Audio::StopMusicStream(music);
    Madokawaii::Platform::Audio::UnloadMusicStream(music);
    Madokawaii::Platform::Audio::CloseAudioDevice();
    return 0;
}