// SPDX-License-Identifier: BSL-1.0

// The sprite editor: cuts a sheet into frames, gives them an origin, and plays
// them back as animations. It does not draw pixels; that is what a pixel
// editor is for.

#include <data/atlas_data.hpp>
#include <render/sprite_atlas.hpp>

#include <imgui.h>
#include <raylib.h>
#include <rlImGui.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace
{

/// Everything the editor is holding. One struct, because a tool this size gains
/// nothing from being spread out, and losing track of what is edited is the
/// only way it can hurt.
struct editor
{
  arpg::sprite_atlas atlas;
  arpg::grid_slice cut;

  /// The sheet, and where it was found. Kept beside the atlas rather than in
  /// it: the atlas names an image, it does not carry one.
  Texture2D sheet{};
  bool sheet_loaded = false;
  std::filesystem::path sheet_path;

  /// Where the atlas is written. Beside the sheet until told otherwise, since
  /// an atlas names its image relative to itself.
  std::string save_path;

  int selected_frame = -1;
  int selected_animation = -1;

  float zoom = 4.0f;
  ImVec2 pan{40.0f, 40.0f};

  bool playing = true;
  float elapsed = 0.0f;

  /// What the last save or load said. Shown until something else happens, so a
  /// refusal cannot go unnoticed.
  std::string report;
  bool report_is_trouble = false;
};

void say(editor& state, std::string message, bool trouble)
{
  state.report = std::move(message);
  state.report_is_trouble = trouble;
}

void take_sheet(editor& state, const std::filesystem::path& path)
{
  if (state.sheet_loaded)
  {
    UnloadTexture(state.sheet);
    state.sheet_loaded = false;
  }

  state.sheet = LoadTexture(path.string().c_str());

  if (state.sheet.id == 0)
  {
    say(state, "cannot read " + path.string(), true);
    return;
  }

  // Nearest, always: a sheet shown smoothed is a sheet whose cuts cannot be
  // placed on a pixel.
  SetTextureFilter(state.sheet, TEXTURE_FILTER_POINT);

  state.sheet_loaded = true;
  state.sheet_path = path;
  state.atlas.image = path.filename().string();

  if (state.save_path.empty())
  {
    state.save_path = (path.parent_path() / (path.stem().string() + ".lua")).string();
  }

  say(state, "sheet " + state.atlas.image + ", " + std::to_string(state.sheet.width) + " by " +
                 std::to_string(state.sheet.height),
      false);
}

void take_atlas(editor& state, const std::filesystem::path& path)
{
  arpg::script_host host;
  const arpg::loaded_atlas read = arpg::load_atlas_from(host, path);

  if (!read.valid())
  {
    say(state, read.error, true);
    return;
  }

  state.atlas = read.value;
  state.save_path = path.string();
  state.selected_frame = state.atlas.frames.empty() ? -1 : 0;
  state.selected_animation = state.atlas.animations.empty() ? -1 : 0;

  // The image is named beside the atlas, so this is where it has to be looked
  // for. An atlas edited on one machine is opened on another.
  take_sheet(state, path.parent_path() / state.atlas.image);
  state.save_path = path.string();

  say(state, "opened " + path.filename().string(), false);
}

void save(editor& state)
{
  if (state.save_path.empty())
  {
    say(state, "nowhere to write to", true);
    return;
  }

  const std::string text = arpg::write_atlas(state.atlas);

  if (text.empty())
  {
    say(state, "something here is named what cannot be written out", true);
    return;
  }

  // Read back before anything touches the disk. A tool that writes a file it
  // cannot open again destroys the work it was used for, and the reader is
  // right here, so there is no excuse not to ask it.
  arpg::script_host host;
  const arpg::loaded_atlas check = arpg::load_atlas(host, text, "what was about to be written");

  if (!check.valid())
  {
    say(state, "refused: " + check.error, true);
    return;
  }

  std::ofstream file(state.save_path, std::ios::binary);

  if (!file)
  {
    say(state, "cannot write " + state.save_path, true);
    return;
  }

  file << text;
  say(state, "written to " + state.save_path, false);
}

// --- the sheet, and the cuts drawn over it ---------------------------------

void draw_sheet(const editor& state)
{
  if (!state.sheet_loaded)
  {
    DrawText("drop a .png to begin, or a .lua to carry on", 40, 40, 20, Color{120, 110, 130, 255});
    return;
  }

  const float zoom = state.zoom;
  const Vector2 at{state.pan.x, state.pan.y};

  DrawTextureEx(state.sheet, at, 0.0f, zoom, WHITE);

  for (std::size_t index = 0; index < state.atlas.frames.size(); ++index)
  {
    const arpg::sprite_frame& frame = state.atlas.frames[index];
    const bool chosen = static_cast<int>(index) == state.selected_frame;

    const Rectangle box{at.x + static_cast<float>(frame.x) * zoom, at.y + static_cast<float>(frame.y) * zoom,
                        static_cast<float>(frame.width) * zoom, static_cast<float>(frame.height) * zoom};

    DrawRectangleLinesEx(box, chosen ? 2.0f : 1.0f,
                         chosen ? Color{226, 205, 154, 255} : Color{120, 110, 130, 160});

    if (!chosen)
    {
      continue;
    }

    // The origin drawn as a cross rather than a dot: it lands between pixels
    // as often as on one, and a dot would hide which side it fell.
    const float ox = box.x + frame.origin.x * zoom;
    const float oy = box.y + frame.origin.y * zoom;

    DrawLineEx(Vector2{ox - 6.0f, oy}, Vector2{ox + 6.0f, oy}, 1.0f, Color{198, 88, 78, 255});
    DrawLineEx(Vector2{ox, oy - 6.0f}, Vector2{ox, oy + 6.0f}, 1.0f, Color{198, 88, 78, 255});
  }
}

/// Picks the frame under the cursor, so a sheet can be worked on by pointing at
/// it rather than by counting down a list.
void pick_frame(editor& state)
{
  if (!state.sheet_loaded || ImGui::GetIO().WantCaptureMouse || !IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
  {
    return;
  }

  const Vector2 mouse = GetMousePosition();
  const float sheet_x = (mouse.x - state.pan.x) / state.zoom;
  const float sheet_y = (mouse.y - state.pan.y) / state.zoom;

  for (std::size_t index = 0; index < state.atlas.frames.size(); ++index)
  {
    const arpg::sprite_frame& frame = state.atlas.frames[index];

    if (sheet_x >= static_cast<float>(frame.x) && sheet_x < static_cast<float>(frame.x + frame.width) &&
        sheet_y >= static_cast<float>(frame.y) && sheet_y < static_cast<float>(frame.y + frame.height))
    {
      state.selected_frame = static_cast<int>(index);
      return;
    }
  }
}

void move_view(editor& state)
{
  if (ImGui::GetIO().WantCaptureMouse)
  {
    return;
  }

  const float wheel = GetMouseWheelMove();

  if (wheel != 0.0f)
  {
    state.zoom = std::clamp(state.zoom + wheel, 1.0f, 24.0f);
  }

  if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
  {
    const Vector2 moved = GetMouseDelta();
    state.pan.x += moved.x;
    state.pan.y += moved.y;
  }
}

// --- the panels -------------------------------------------------------------

void sheet_panel(editor& state)
{
  ImGui::Begin("sheet");

  ImGui::TextUnformatted(state.atlas.image.empty() ? "(none)" : state.atlas.image.c_str());

  if (state.sheet_loaded)
  {
    ImGui::Text("%d by %d", state.sheet.width, state.sheet.height);
  }

  ImGui::SliderFloat("zoom", &state.zoom, 1.0f, 24.0f, "%.0f");
  ImGui::TextUnformatted("wheel zooms, right drag pans");

  ImGui::Separator();

  ImGui::InputInt("cell width", &state.cut.cell_width);
  ImGui::InputInt("cell height", &state.cut.cell_height);
  ImGui::InputInt("margin", &state.cut.margin);
  ImGui::InputInt("spacing", &state.cut.spacing);

  std::string prefix = state.cut.prefix;
  prefix.resize(64);

  if (ImGui::InputText("prefix", prefix.data(), prefix.size()))
  {
    state.cut.prefix = prefix.c_str();
  }

  ImGui::SliderFloat2("origin", &state.cut.origin.x, 0.0f, 1.0f, "%.2f");

  if (ImGui::Button("cut on the grid") && state.sheet_loaded)
  {
    // The animations are left alone even though the frames they name may be
    // gone: the save reads back what it writes, and will say so rather than
    // throwing away work here on a guess.
    state.atlas.frames = arpg::slice_grid(state.sheet.width, state.sheet.height, state.cut);
    state.selected_frame = state.atlas.frames.empty() ? -1 : 0;
    say(state, "cut into " + std::to_string(state.atlas.frames.size()) + " frames", false);
  }

  ImGui::Separator();

  std::string where = state.save_path;
  where.resize(512);

  if (ImGui::InputText("file", where.data(), where.size()))
  {
    state.save_path = where.c_str();
  }

  if (ImGui::Button("save") || (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S)))
  {
    save(state);
  }

  if (!state.report.empty())
  {
    ImGui::Separator();
    ImGui::PushStyleColor(ImGuiCol_Text, state.report_is_trouble ? ImVec4{0.85f, 0.4f, 0.4f, 1.0f}
                                                                 : ImVec4{0.6f, 0.8f, 0.6f, 1.0f});
    ImGui::TextWrapped("%s", state.report.c_str());
    ImGui::PopStyleColor();
  }

  ImGui::End();
}

void frames_panel(editor& state)
{
  ImGui::Begin("frames");

  for (std::size_t index = 0; index < state.atlas.frames.size(); ++index)
  {
    const bool chosen = static_cast<int>(index) == state.selected_frame;

    if (ImGui::Selectable(state.atlas.frames[index].name.c_str(), chosen))
    {
      state.selected_frame = static_cast<int>(index);
    }
  }

  ImGui::End();

  ImGui::Begin("frame");

  if (state.selected_frame < 0 || state.selected_frame >= static_cast<int>(state.atlas.frames.size()))
  {
    ImGui::TextUnformatted("nothing chosen");
    ImGui::End();
    return;
  }

  arpg::sprite_frame& frame = state.atlas.frames[static_cast<std::size_t>(state.selected_frame)];

  std::string name = frame.name;
  name.resize(64);

  if (ImGui::InputText("name", name.data(), name.size()))
  {
    frame.name = name.c_str();
  }

  ImGui::InputInt("x", &frame.x);
  ImGui::InputInt("y", &frame.y);
  ImGui::InputInt("width", &frame.width);
  ImGui::InputInt("height", &frame.height);

  ImGui::Separator();
  ImGui::DragFloat2("origin", &frame.origin.x, 0.25f);

  if (ImGui::Button("origin to the feet"))
  {
    frame.origin = arpg::vec2{static_cast<float>(frame.width) * 0.5f, static_cast<float>(frame.height)};
  }

  ImGui::SameLine();

  if (ImGui::Button("origin to the middle"))
  {
    frame.origin = arpg::vec2{static_cast<float>(frame.width) * 0.5f, static_cast<float>(frame.height) * 0.5f};
  }

  ImGui::End();
}

void animation_panel(editor& state)
{
  ImGui::Begin("animations");

  if (ImGui::Button("new"))
  {
    arpg::sprite_animation clip;
    clip.name = "clip_" + std::to_string(state.atlas.animations.size());
    state.atlas.animations.push_back(clip);
    state.selected_animation = static_cast<int>(state.atlas.animations.size()) - 1;
    state.elapsed = 0.0f;
  }

  for (std::size_t index = 0; index < state.atlas.animations.size(); ++index)
  {
    const bool chosen = static_cast<int>(index) == state.selected_animation;

    if (ImGui::Selectable(state.atlas.animations[index].name.c_str(), chosen))
    {
      state.selected_animation = static_cast<int>(index);
      state.elapsed = 0.0f;
    }
  }

  ImGui::End();

  ImGui::Begin("animation");

  if (state.selected_animation < 0 || state.selected_animation >= static_cast<int>(state.atlas.animations.size()))
  {
    ImGui::TextUnformatted("nothing chosen");
    ImGui::End();
    return;
  }

  arpg::sprite_animation& clip = state.atlas.animations[static_cast<std::size_t>(state.selected_animation)];

  std::string name = clip.name;
  name.resize(64);

  if (ImGui::InputText("name", name.data(), name.size()))
  {
    clip.name = name.c_str();
  }

  ImGui::Checkbox("loops", &clip.loops);

  if (ImGui::Button("remove this animation"))
  {
    state.atlas.animations.erase(state.atlas.animations.begin() + state.selected_animation);
    state.selected_animation = state.atlas.animations.empty() ? -1 : 0;
    ImGui::End();
    return;
  }

  ImGui::Separator();

  const bool has_frame = state.selected_frame >= 0 && state.selected_frame < static_cast<int>(state.atlas.frames.size());

  if (ImGui::Button("add the chosen frame") && has_frame)
  {
    clip.frames.push_back(
        arpg::animation_frame{state.atlas.frames[static_cast<std::size_t>(state.selected_frame)].name, 0.1f});
  }

  int remove = -1;
  int lift = -1;

  for (std::size_t step = 0; step < clip.frames.size(); ++step)
  {
    ImGui::PushID(static_cast<int>(step));

    ImGui::TextUnformatted(clip.frames[step].frame.c_str());
    ImGui::SameLine(180.0f);
    ImGui::SetNextItemWidth(90.0f);
    ImGui::DragFloat("s", &clip.frames[step].seconds, 0.005f, 0.001f, 10.0f, "%.3f");

    ImGui::SameLine();

    if (ImGui::SmallButton("up"))
    {
      lift = static_cast<int>(step);
    }

    ImGui::SameLine();

    if (ImGui::SmallButton("x"))
    {
      remove = static_cast<int>(step);
    }

    ImGui::PopID();
  }

  if (lift > 0)
  {
    std::swap(clip.frames[static_cast<std::size_t>(lift)], clip.frames[static_cast<std::size_t>(lift) - 1]);
  }

  if (remove >= 0)
  {
    clip.frames.erase(clip.frames.begin() + remove);
  }

  ImGui::Separator();
  ImGui::Checkbox("play", &state.playing);
  ImGui::SameLine();
  ImGui::Text("%.2f s", static_cast<double>(arpg::duration_of(clip)));

  ImGui::End();
}

/// Draws the chosen animation at its real speed, which is the only way to tell
/// whether a hold is too long.
void draw_preview(editor& state)
{
  if (!state.sheet_loaded || state.selected_animation < 0 ||
      state.selected_animation >= static_cast<int>(state.atlas.animations.size()))
  {
    return;
  }

  const arpg::sprite_animation& clip = state.atlas.animations[static_cast<std::size_t>(state.selected_animation)];

  if (clip.frames.empty())
  {
    return;
  }

  const arpg::sprite_frame* frame = state.atlas.find_frame(clip.frames[arpg::frame_at(clip, state.elapsed)].frame);

  if (frame == nullptr)
  {
    return;
  }

  const float zoom = 6.0f;
  const Vector2 at{static_cast<float>(GetScreenWidth()) - 220.0f, static_cast<float>(GetScreenHeight()) - 220.0f};

  DrawRectangleLines(static_cast<int>(at.x) - 8, static_cast<int>(at.y) - 8, 200, 200, Color{86, 72, 102, 255});

  const Rectangle source{static_cast<float>(frame->x), static_cast<float>(frame->y), static_cast<float>(frame->width),
                         static_cast<float>(frame->height)};

  const Rectangle target{at.x + 100.0f - frame->origin.x * zoom, at.y + 150.0f - frame->origin.y * zoom,
                         static_cast<float>(frame->width) * zoom, static_cast<float>(frame->height) * zoom};

  // The origin is put on a fixed spot, so a frame drawn with the wrong one
  // jumps against the others rather than hiding among them.
  DrawTexturePro(state.sheet, source, target, Vector2{0.0f, 0.0f}, 0.0f, WHITE);
  DrawLineEx(Vector2{at.x + 80.0f, at.y + 150.0f}, Vector2{at.x + 120.0f, at.y + 150.0f}, 1.0f,
             Color{198, 88, 78, 255});
}

void take_dropped_files(editor& state)
{
  if (!IsFileDropped())
  {
    return;
  }

  const FilePathList dropped = LoadDroppedFiles();

  for (unsigned int index = 0; index < dropped.count; ++index)
  {
    const std::filesystem::path path{dropped.paths[index]};

    if (path.extension() == ".lua")
    {
      take_atlas(state, path);
    }
    else
    {
      take_sheet(state, path);
    }
  }

  UnloadDroppedFiles(dropped);
}

} // namespace

int main(int argc, char** argv)
{
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(1280, 800, "arpege sprite editor");
  SetTargetFPS(60);
  rlImGuiSetup(true);

  editor state;

  for (int index = 1; index < argc; ++index)
  {
    const std::filesystem::path path{argv[index]};

    if (path.extension() == ".lua")
    {
      take_atlas(state, path);
    }
    else
    {
      take_sheet(state, path);
    }
  }

  while (!WindowShouldClose())
  {
    take_dropped_files(state);
    move_view(state);
    pick_frame(state);

    if (state.playing)
    {
      state.elapsed += GetFrameTime();
    }

    BeginDrawing();
    ClearBackground(Color{24, 22, 30, 255});

    draw_sheet(state);
    draw_preview(state);

    rlImGuiBegin();
    sheet_panel(state);
    frames_panel(state);
    animation_panel(state);
    rlImGuiEnd();

    EndDrawing();
  }

  if (state.sheet_loaded)
  {
    UnloadTexture(state.sheet);
  }

  rlImGuiShutdown();
  CloseWindow();

  return 0;
}
