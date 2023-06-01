#include <iostream>
#include <fstream>
#include <unordered_set>

#include <imgui.h>
#include <imgui-SFML.h>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>

#include "FileBrowser.h"
#include "DebugState.h"
#include "Chip8.h"
#include "Opcode.h"

class Game
{
private:
	sf::RenderWindow window;

	sf::Color setColor;
	sf::Color unsetColor;

	bool beeping;
	sf::Sound beep;
	sf::SoundBuffer beepBuffer;

	const char* currentRomPath = nullptr;
	int currentRomLength;

	std::unordered_set<int> breakpoints;

	DebugState state;
	Chip8 cpu;

public:
	Game()
		: window(sf::VideoMode(640, 640), "Chip-8 Emulator"), state(), cpu()
	{
		window.setVerticalSyncEnabled(true);
		window.resetGLStates();

		ImGui::SFML::Init(window);
		ImGui::GetStyle().WindowRounding = 0.0f;
		ImGui::GetStyle().ChildRounding = 0.0f;
		ImGui::GetStyle().FrameRounding = 0.0f;
		ImGui::GetStyle().GrabRounding = 0.0f;
		ImGui::GetStyle().PopupRounding = 0.0f;
		ImGui::GetStyle().ScrollbarRounding = 0.0f;
	}

	~Game()
	{
		ImGui::SFML::Shutdown();
	}

	void Run()
	{
		Setup();

		sf::Clock clock;
		while (window.isOpen())
		{
			sf::Event event;
			while (window.pollEvent(event))
			{
				ImGui::SFML::ProcessEvent(event);

				if (event.type == sf::Event::Closed)
				{
					window.close();
				}
				else if (event.type == sf::Event::Resized)
				{
					window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
				}
				else
				{
					Process(event);
				}
			}

			window.clear();
			ImGui::SFML::Update(window, clock.restart());

			Update();

			ImGui::SFML::Render(window);
			window.display();
		}
	}

private:
	void Setup()
	{
		setColor = sf::Color::White;
		unsetColor = sf::Color::Black;

		PrepareBeep();

		state.CapFramerate = true;
		state.FocusMode = false;
	}

	void Update()
	{
		RenderLoadPopup();

		RenderMenu();
		RenderDisplay();

		if (!state.FocusMode)
		{
			RenderCpuState();
			RenderProgram();
		}

		HandleAudio();
		HandleInput();

		if (state.IsRomLoaded && !state.IsPaused)
		{
			cpu.ClockCycle();

			if (breakpoints.find(cpu.ProgramCounter) != breakpoints.end())
				state.IsPaused = true;
		}
	}

	void Process(sf::Event& event)
	{
		if (ImGui::GetIO().WantCaptureKeyboard)
			return;

		if (event.type == sf::Event::KeyPressed)
		{
			switch (event.key.code)
			{
			case sf::Keyboard::F5:
				state.IsPaused = !state.IsPaused;
				break;

			case sf::Keyboard::F10:
				cpu.ClockCycle();
				break;
			}
		}
	}

	void RenderMenu()
	{
		bool loadFilePopup = false;

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Insert Rom"))
				{
					loadFilePopup = true;
				}

				if (ImGui::MenuItem("Reset Rom"))
				{
					cpu.UnloadRom();
					LoadRomFile();
				}

				if (ImGui::MenuItem("Eject Rom"))
				{
					cpu.UnloadRom();
					state.IsRomLoaded = false;
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Save State"))
				{
					state.IsStateSaved = true;
					DumpCpuData();
				}

				if (ImGui::MenuItem("Load State"))
				{
					if (state.IsStateSaved)
						LoadCpuData();
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Debug"))
			{
				if (ImGui::MenuItem("Pause", "F5"))
				{
					state.IsPaused = true;
				}

				if (ImGui::MenuItem("Step", "F10"))
				{
					cpu.ClockCycle();
				}

				if (ImGui::MenuItem("Resume", "F5"))
				{
					state.IsPaused = false;
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Set Breakpoints"))
				{
					breakpoints.emplace(cpu.ProgramCounter);
				}

				if (ImGui::MenuItem("Clear Breakpoints"))
				{
					breakpoints.clear();
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Prefs"))
			{
				float setBuffer[3] = {setColor.r / 255.0f, setColor.g / 255.0f, setColor.b / 255.0f};
				if (ImGui::ColorEdit3("Set Pixels", setBuffer))
				{
					setColor = sf::Color(setBuffer[0] * 255, setBuffer[1] * 255, setBuffer[2] * 255);
				}

				float unsetBuffer[3] = {unsetColor.r / 255.0f, unsetColor.g / 255.0f, unsetColor.b / 255.0f};
				if (ImGui::ColorEdit3("Unset Pixels", unsetBuffer))
				{
					unsetColor = sf::Color(unsetBuffer[0] * 255, unsetBuffer[1] * 255, unsetBuffer[2] * 255);
				}

				if (ImGui::Checkbox("Cap Framerate", &state.CapFramerate))
				{
					window.setVerticalSyncEnabled(state.CapFramerate);
				}

				if (ImGui::Checkbox("Focus Mode", &state.FocusMode))
				{
					window.setSize(state.FocusMode ? sf::Vector2u(640, 340) : sf::Vector2u(640, 640));
				}

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		if (loadFilePopup)
		{
			if (currentRomPath)
			{
				std::string currentRomDir = std::filesystem::path(currentRomPath).remove_filename().u8string();
				ImGui::InitFileBrowser(currentRomDir);
			}
			else
			{
				ImGui::InitFileBrowser("");
			}
		}
	}

	void RenderDisplay()
	{
		sf::RectangleShape pixel({10.0f, 10.0f});

		for (int y = 0; y < 32; y++)
		{
			for (int x = 0; x < 64; x++)
			{
				bool isSet = cpu.Graphics[y * 64 + x];

				pixel.setPosition({x * 10.0f, y * 10.0f + 20.0f});
				pixel.setFillColor(isSet ? setColor : unsetColor);

				window.draw(pixel);
			}
		}
	}

	void RenderCpuState()
	{
		ImGui::SetNextWindowPos({0, 340});
		ImGui::SetNextWindowSize({320, 300});
		ImGui::Begin("CPU State", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

		const char* targets[] = {"Memory", "Registers", "Stack", "Graphics", "Keyboard"};
		static int selectedTarget = 0;

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::Combo("##target", &selectedTarget, targets, sizeof(targets) / sizeof(char*));

		switch (selectedTarget)
		{
		case 0:
			RenderMemory();
			break;
		case 1:
			RenderRegisters();
			break;
		case 2:
			RenderStack();
			break;
		case 3:
			RenderGraphics();
			break;
		case 4:
			RenderKeyboard();
			break;
		}

		ImGui::End();
	}

	void RenderMemory()
	{
		ImGui::Dummy({0, 10});

		static int memoryStart = 0;
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::DragInt("##memory_start", &memoryStart, 0.5f, 0, 4096 - 32, "%04X", ImGuiSliderFlags_NoRoundToFormat);

		if (ImGui::BeginTable("##memory_table", 4))
		{
			for (int offset = 0; offset < 32; offset++)
			{
				ImGui::TableNextColumn();

				ImGui::AlignTextToFramePadding();
				ImGui::Text("%04X:", memoryStart + offset);
				ImGui::SameLine();

				ImGui::PushID(offset);
				ImGui::InputScalar("##byte", ImGuiDataType_U8, &cpu.Memory[memoryStart + offset], (void*)1, (void*)32, "%X", ImGuiInputTextFlags_CharsHexadecimal);
				ImGui::PopID();
			}

			ImGui::EndTable();
		}
	}

	void RenderRegisters()
	{
		if (ImGui::BeginTable("##register_table", 4))
		{
			for (int reg = 0; reg < 16; reg++)
			{
				ImGui::TableNextColumn();

				ImGui::AlignTextToFramePadding();
				ImGui::Text("V%02X:", reg);
				ImGui::SameLine();

				ImGui::PushID(reg);
				ImGui::InputScalar("##byte", ImGuiDataType_U8, &cpu.Registers[reg], (void*)1, (void*)32, "%X", ImGuiInputTextFlags_CharsHexadecimal);
				ImGui::PopID();
			}

			ImGui::EndTable();
		}

		ImGui::Dummy({0, 10});
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Index:");
		ImGui::SameLine();
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::InputScalar("##index_reg", ImGuiDataType_U16, &cpu.IndexRegister, 0, 0, "%X", ImGuiInputTextFlags_CharsHexadecimal);

		ImGui::Dummy({0, 10});

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Delay Timer:");
		ImGui::SameLine();
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::InputScalar("##delay_timer", ImGuiDataType_U8, &cpu.DelayTimer, 0, 0, "%d");

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Sound Timer:");
		ImGui::SameLine();
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::InputScalar("##sound_timer", ImGuiDataType_U8, &cpu.SoundTimer, 0, 0, "%d");
	}

	void RenderStack()
	{
		auto renderStackItem = [&](int i)
		{
			ImGui::TableSetColumnIndex(i > 7 ? 0 : 1);

			if (i == cpu.StackPointer)
				ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, sf::Color::Red.toInteger());

			if (cpu.Stack[i] != 0)
				ImGui::Text("%02d: %X", i, cpu.Stack[i]);
			else
				ImGui::Text("%02d:", i);
		};

		ImGui::Dummy({0, 10});

		ImGui::BeginTable("stack", 2, ImGuiTableFlags_Borders);
		for (int i = 7; i >= 0; i--)
		{
			ImGui::TableNextRow();
			renderStackItem(i);
			renderStackItem(i + 8);
		}
		ImGui::EndTable();

		ImGui::Dummy({0, 10});

		int min = 0, max = 16 - 1;
		ImGui::SliderScalar("Stack Pointer", ImGuiDataType_U8, &cpu.StackPointer, &min, &max, "%d");
	}

	void RenderGraphics()
	{
		ImGui::Dummy({0, 10});

		static int x = 0;
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::DragInt("##pixel_start", &x, 0.1f, 0, 64 - 1, "%d");

		if (ImGui::BeginTable("##pixel_table", 4))
		{
			for (int y = 0; y < 32; y++)
			{
				ImGui::TableNextColumn();

				ImGui::AlignTextToFramePadding();
				ImGui::Text("(%02d,%02d):", x, y);
				ImGui::SameLine(ImGui::GetContentRegionAvail().x - 10);

				ImGui::PushID(y * 64 + x);
				ImGui::Checkbox("##pixel", &cpu.Graphics[x + y * 64 + x]);
				ImGui::PopID();
			}

			ImGui::EndTable();
		}
	}

	void RenderKeyboard()
	{
		ImGui::Dummy({0, 10});

		const char* original = "123C456D789EA0BF";
		const char* mapped = "1234QWERASDFZXCV";

		if (ImGui::BeginTable("##input_table", 4, ImGuiTableFlags_Borders))
		{
			for (int i = 0; i < 16; i++)
			{
				ImGui::TableNextColumn();

				if (cpu.Keyboard[i])
					ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, sf::Color::Red.toInteger());

				ImGui::Dummy({0, 10});
				ImGui::Text("  %c [%c] ", original[i], mapped[i]);
				ImGui::Dummy({0, 10});
			}

			ImGui::EndTable();
		}
	}

	void RenderProgram()
	{
		ImGui::SetNextWindowPos({320, 340});
		ImGui::SetNextWindowSize({320, 300});
		ImGui::Begin("Program", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

		static int scrollValue = 512;
		ImGui::DragInt("##scroll", &scrollValue, 2, PROGRAM_START, PROGRAM_START + currentRomLength);

		ImGui::SameLine();

		static bool lock = true;
		ImGui::Checkbox("##lock_scroll", &lock);
		if (lock)
			scrollValue = cpu.ProgramCounter;

		ImGui::BeginTable("##program_table", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp);
		ImGui::TableSetupColumn("##col_opcode", 0, 0.1f);
		ImGui::TableSetupColumn("##col_opcode", 0, 0.15f);
		ImGui::TableSetupColumn("##col_addr", 0, 0.15f);
		ImGui::TableSetupColumn("##col_desc", 0, 0.6f);

		for (int i = 0; i < 12; i += 2)
		{
			ImGui::TableNextRow();
			if (cpu.ProgramCounter == scrollValue + i)
				ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, sf::Color::Red.toInteger());

			int address = (scrollValue + i) % 4095;
			word instruction = cpu.Memory[address] << 8 | cpu.Memory[address + 1];
			Opcode code = Opcodes::Match(instruction);

			ImGui::TableSetColumnIndex(0);
			ImGui::PushID(address);
			bool breakpoint = std::find(breakpoints.begin(), breakpoints.end(), address) != breakpoints.end();
			if (ImGui::Checkbox("##breakpoint", &breakpoint))
			{
				if (breakpoint)
					breakpoints.emplace(address);
				else
					breakpoints.erase(address);
			}
			ImGui::PopID();

			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%04X", address);

			ImGui::TableSetColumnIndex(2);
			ImGui::TextWrapped("%04X", instruction);

			ImGui::TableSetColumnIndex(3);
			ImGui::TextWrapped(code.Description);
		}

		ImGui::EndTable();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Program Counter:");
		ImGui::SameLine();
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::DragScalar("##program_counter", ImGuiDataType_U16, &cpu.ProgramCounter, 2, 0, 0, "%d");

		ImGui::End();
	}

	void RenderLoadPopup()
	{
		auto callback = [&](const char* file)
		{
			currentRomPath = file;
			LoadRomFile();

			if (std::filesystem::exists(std::filesystem::path(currentRomPath).replace_extension(".c8ss")))
				state.IsStateSaved = true;
		};
		ImGui::FileBrowser(callback);
	}

	void HandleInput()
	{
		ImGuiIO io = ImGui::GetIO();

		if (!io.WantCaptureMouse)
		{
			sf::Vector2i mouse = sf::Mouse::getPosition(window);
			for (int y = 0; y < 32; y++)
			{
				for (int x = 0; x < 64; x++)
				{
					if (sf::IntRect(x * 10, y * 10 + 20, 10, 10).contains(mouse))
					{
						if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
							cpu.Graphics[y * 64 + x] = true;

						if (sf::Mouse::isButtonPressed(sf::Mouse::Right))
							cpu.Graphics[y * 64 + x] = false;
					}
				}
			}
		}

		if (!io.WantCaptureKeyboard)
		{
			cpu.Keyboard[0] = sf::Keyboard::isKeyPressed(sf::Keyboard::Num1);
			cpu.Keyboard[1] = sf::Keyboard::isKeyPressed(sf::Keyboard::Num2);
			cpu.Keyboard[2] = sf::Keyboard::isKeyPressed(sf::Keyboard::Num3);
			cpu.Keyboard[3] = sf::Keyboard::isKeyPressed(sf::Keyboard::Num4);

			cpu.Keyboard[4] = sf::Keyboard::isKeyPressed(sf::Keyboard::Q);
			cpu.Keyboard[5] = sf::Keyboard::isKeyPressed(sf::Keyboard::W);
			cpu.Keyboard[6] = sf::Keyboard::isKeyPressed(sf::Keyboard::E);
			cpu.Keyboard[7] = sf::Keyboard::isKeyPressed(sf::Keyboard::R);

			cpu.Keyboard[8] = sf::Keyboard::isKeyPressed(sf::Keyboard::A);
			cpu.Keyboard[9] = sf::Keyboard::isKeyPressed(sf::Keyboard::S);
			cpu.Keyboard[10] = sf::Keyboard::isKeyPressed(sf::Keyboard::D);
			cpu.Keyboard[11] = sf::Keyboard::isKeyPressed(sf::Keyboard::F);

			cpu.Keyboard[12] = sf::Keyboard::isKeyPressed(sf::Keyboard::Z);
			cpu.Keyboard[13] = sf::Keyboard::isKeyPressed(sf::Keyboard::X);
			cpu.Keyboard[14] = sf::Keyboard::isKeyPressed(sf::Keyboard::C);
			cpu.Keyboard[15] = sf::Keyboard::isKeyPressed(sf::Keyboard::V);
		}
	}

	void HandleAudio()
	{
		if (beeping && cpu.SoundTimer == 0)
		{
			beeping = false;
			beep.stop();
		}

		if (!beeping && cpu.SoundTimer > 0)
		{
			beeping = true;
			beep.play();
		}
	}

	void DumpCpuData()
	{
		std::string tempName = std::filesystem::path(currentRomPath).replace_extension(".c8ss").u8string();

		std::ofstream file(tempName, std::ios::out | std::ios::binary);
		file.write((char*)&cpu, sizeof(cpu));
		file.close();
	}

	void LoadCpuData()
	{
		std::string tempName = std::filesystem::path(currentRomPath).replace_extension(".c8ss").u8string();

		std::ifstream file(tempName, std::ios::in | std::ios::binary);
		file.read((char*)&cpu, sizeof(cpu));
		file.close();
	}

	void LoadRomFile()
	{
		std::ifstream file(currentRomPath, std::ios::in | std::ios::binary);

		file.seekg(0, file.end);
		currentRomLength = file.tellg();

		byte* code = new byte[currentRomLength];

		file.seekg(0, file.beg);
		file.read((char*)code, currentRomLength);

		cpu.LoadRom(code, currentRomLength);
		state.IsRomLoaded = true;

		delete[] code;

		file.close();
	}

	void PrepareBeep()
	{
		const int SAMPLE_RATE = 44100;
		const int FREQUENCY = 880;
		const int AMPLITUDE = 16000;

		const int SAMPLE_COUNT = SAMPLE_RATE / FREQUENCY;
		short wavetable[SAMPLE_COUNT];

		float stepSize = (6.284f) / SAMPLE_COUNT;
		for (int i = 0; i < SAMPLE_COUNT; i++)
		{
			wavetable[i] = (short)(sinf(stepSize * i) * AMPLITUDE);
		}

		beepBuffer.loadFromSamples(wavetable, SAMPLE_COUNT, 1, SAMPLE_RATE);

		beep.setBuffer(beepBuffer);
		beep.setLoop(true);
	}
};

int main()
{
	Game game;
	game.Run();
}