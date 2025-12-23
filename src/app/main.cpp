
#include <algorithm>
#include <ctime>
#include <format>
#include <string>
#include <vector>

#include "Madokawaii/app/app_config.h"
#include "Madokawaii/app/def.h"
#include "Madokawaii/app/chart.h"
#include "Madokawaii/app/line_operation.h"
#include "Madokawaii/app/note_operation.h"
#include "Madokawaii/app/res_pack.h"
#include "Madokawaii/platform/audio.h"
#include "Madokawaii/platform/log.h"
#include "Madokawaii/platform/core.h"
#include "Madokawaii/platform/graphics.h"

struct AppContext {
    int screenWidth{1920};
    int screenHeight{1080};
    Madokawaii::Platform::Audio::Music music{};
    Madokawaii::App::chart mainChart{};
    bool initialized{false};
    std::shared_ptr<Madokawaii::App::ResPack::ResPack> global_respack;
};

extern "C" {

int AppInit(void*& appstate) {
    appstate = new AppContext;
    auto& ctx = *static_cast<AppContext*>(appstate);
    Madokawaii::Platform::Audio::InitAudioDevice();

    auto& danli = Madokawaii::AppConfig::ConfigManager::Instance();
    const auto& musicPath = danli.GetMusicPath();
    const auto& chartPath = danli.GetChartPath();
    const auto& resPackPath = danli.GetResPackPath();

    clock_t begin = clock();
    if (!Madokawaii::Platform::Core::FileExists(musicPath.c_str())) {
        Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_ERROR, "MAIN: Music file does not exist!");
        return false;
    }

    ctx.music = Madokawaii::Platform::Audio::LoadMusicStream(musicPath.c_str());
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "MAIN: Music stream loaded");

    ctx.mainChart = Madokawaii::App::Chart::LoadChartFromFile(chartPath.c_str());
    if (!Madokawaii::App::Chart::IsValidChart(ctx.mainChart)) {
        Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_ERROR, "MAIN: Failed to load chart!");
        return false;
    }

    int dataSize = 0;
    auto respack_mem_stream = Madokawaii::Platform::Core::LoadFileData(resPackPath.c_str(), &dataSize);
    if (!respack_mem_stream)
    {
        Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_ERROR, "MAIN: Failed to load respack file into memory!");
        return false;
    }
    ctx.global_respack = Madokawaii::App::ResPack::LoadResPackFromMemoryStream(respack_mem_stream, dataSize);
	Madokawaii::Platform::Core::UnloadFileData(respack_mem_stream);

    clock_t end = clock();
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "MAIN: Chart Initialization Successful!");
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "    > Format Version:         %d", ctx.mainChart.formatVersion);
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "    > Number of notes:        %d", ctx.mainChart.numOfNotes);
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "    > Number of judgelines:   %d", ctx.mainChart.judgelineCount);
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "    > SpeedEvents Count:      %d", ctx.mainChart.speedEventsCount);
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "    > DisappearEvents Count:  %d", ctx.mainChart.disappearEventCount);
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "    > MoveEvents Count:       %d", ctx.mainChart.moveEventCount);
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "    > RotateEvents Count:     %d", ctx.mainChart.rotateEventCount);
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "MAIN: Elapsed time: %lf s", (end - begin) * 1.0 / CLOCKS_PER_SEC);

    Madokawaii::App::Chart::InitializeJudgelines(ctx.mainChart);

    Madokawaii::Platform::Core::InitWindow(ctx.screenWidth, ctx.screenHeight, "Madokawaii");

    InitializeNoteRenderer(*ctx.global_respack, ctx.screenWidth, ctx.screenHeight);
    ctx.music.looping = false;
    auto musicLength = Madokawaii::Platform::Audio::GetMusicTimeLength(ctx.music);
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "MAIN: Music Length: %f", musicLength);
    Madokawaii::Platform::Audio::PlayMusicStream(ctx.music);

    ctx.initialized = true;
    return ctx.initialized;
}

int AppIterate(void * appstate) {
    auto& ctx = *static_cast<AppContext*>(appstate);
    if (!ctx.initialized)
        return -1;

    Madokawaii::Platform::Audio::UpdateMusicStream(ctx.music);
    Madokawaii::Platform::Graphics::BeginDrawing();
    Madokawaii::Platform::Graphics::ClearBackground(Madokawaii::Platform::Graphics::M_BLACK);

    auto thisFrameTime = Madokawaii::Platform::Audio::GetMusicTimePlayed(ctx.music);
    if (!Madokawaii::Platform::Audio::IsMusicStreamPlaying(ctx.music)) {
        Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "MAIN: Music playback end");
        Madokawaii::Platform::Graphics::EndDrawing();
        return false;
    }

    RenderHoldCallback(thisFrameTime, ctx.mainChart);
    auto noteRenderList = std::vector<Madokawaii::App::chart::judgeline::note *>();

    for (auto &judgeline: ctx.mainChart.judgelines) {
        UpdateJudgeline(judgeline, thisFrameTime, ctx.screenWidth, ctx.screenHeight, noteRenderList);
        RenderJudgeline(judgeline, ctx.screenWidth, ctx.screenHeight);

    }

    for (auto notePtrIter = noteRenderList.begin(); notePtrIter != noteRenderList.end(); ++notePtrIter) {
        for (auto notePtrIter1 = notePtrIter + 1; notePtrIter1 != noteRenderList.end(); ++notePtrIter1) {
            if (fabs((*notePtrIter)->realTime - (*notePtrIter1)->realTime) < 1e-6) {
                (*notePtrIter1)->isMultipleNote = (*notePtrIter)->isMultipleNote = true;
            } else if ((*notePtrIter1)->realTime > (*notePtrIter)->realTime) {
                break;
            }
        }
    }


    for (auto notePtr: noteRenderList) {
        // TODO: 实现note渲染
        RenderNote(*notePtr);
    }

    RenderDebugInfo();
    Madokawaii::Platform::Graphics::EndDrawing();

    return !Madokawaii::Platform::Core::WindowShouldClose();
}

int AppExit(void * appstate) {
    auto& ctx = *static_cast<AppContext*>(appstate);
    Madokawaii::Platform::Core::CloseWindow();
    if (Madokawaii::Platform::Audio::IsMusicStreamPlaying(ctx.music))
        Madokawaii::Platform::Audio::StopMusicStream(ctx.music);
    Madokawaii::Platform::Audio::UnloadMusicStream(ctx.music);
    Madokawaii::Platform::Audio::CloseAudioDevice();
    delete static_cast<AppContext*>(appstate);
    return 0;
}

}

/*
int main() {
    constexpr int screenWidth = 1920;
    constexpr int screenHeight = 1080;

    Madokawaii::Platform::Audio::InitAudioDevice();

    auto& danli = Madokawaii::AppConfig::ConfigManager::Instance();
    const auto& musicPath = danli.GetMusicPath(), chartPath = danli.GetChartPath();

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

    music.looping = false;
    auto musicLength = Madokawaii::Platform::Audio::GetMusicTimeLength(music);
    Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "MAIN: Music Length: %f", musicLength);
    Madokawaii::Platform::Audio::PlayMusicStream(music);

    while (!Madokawaii::Platform::Core::WindowShouldClose()) {
        Madokawaii::Platform::Audio::UpdateMusicStream(music);
        Madokawaii::Platform::Graphics::BeginDrawing();
        Madokawaii::Platform::Graphics::ClearBackground(Madokawaii::Platform::Graphics::M_BLACK);

        auto thisFrameTime = Madokawaii::Platform::Audio::GetMusicTimePlayed(music);
        if (!Madokawaii::Platform::Audio::IsMusicStreamPlaying(music)) {
            Madokawaii::Platform::Log::TraceLog(Madokawaii::Platform::Log::TraceLogLevel::LOG_INFO, "MAIN: Music playback end");
            Madokawaii::Platform::Graphics::EndDrawing();
            break;
        }
        auto noteRenderList = std::vector<Madokawaii::Defs::chart::judgeline::note *>();

        // update judgeline & render
        for (auto &judgeline: mainChart.judgelines) {

            UpdateJudgeline(judgeline, thisFrameTime, screenWidth, screenHeight, noteRenderList);

            // line render
            RenderJudgeline(judgeline, screenWidth, screenHeight);

            // note update
            judgeline.info.positionY = judgeline.info.speedEventPointer->floorPosition + (thisFrameTime -
                Madokawaii::Defs::Chart::CalcRealTime(
                    judgeline.bpm, static_cast<int>(judgeline.info.speedEventPointer->startTime)))
            * judgeline.info.speedEventPointer->value;
            
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

        RenderDebugInfo();
        Madokawaii::Platform::Graphics::EndDrawing();
    }

    Madokawaii::Platform::Core::CloseWindow();
    if (Madokawaii::Platform::Audio::IsMusicStreamPlaying(music))
        Madokawaii::Platform::Audio::StopMusicStream(music);
    Madokawaii::Platform::Audio::UnloadMusicStream(music);
    Madokawaii::Platform::Audio::CloseAudioDevice();
    return 0;
}
*/