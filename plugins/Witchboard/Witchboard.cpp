#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <new>

#include <distingnt/api.h>
#include <distingnt/serialisation.h>

#ifndef _NT_DRAM_SECTION
#define _NT_DRAM_SECTION
#endif

#ifndef WITCHBOARD_ENABLE_DRAM_CODE
#undef _NT_DRAM_SECTION
#define _NT_DRAM_SECTION
#endif

namespace
{

constexpr int kMaxChannels = 10;
constexpr int kNumRoutes = 8;
constexpr int kNumInserts = 2;
constexpr int kNumRouteParams = 6;
constexpr int kNumInsertStates = 4;
constexpr int kInsertParameterMax = 4;
constexpr int kNumOutputPairs = 4 + kNumRoutes;
constexpr int kHardwareNameLength = 24;
constexpr int kSlotNameLength = 20;

constexpr int kInsertDry = 0;

enum RouteField
{
	kRouteOutputL,
	kRouteOutputR,
	kRouteReturnL,
	kRouteReturnR,
	kRouteSendWidth,
	kRouteReturnWidth,
};

enum OutputPath
{
	kOutputPathMain,
	kOutputPathBypass,
};

enum Width
{
	kWidthMono,
	kWidthStereo,
};

enum MasterMode
{
	kMasterSplit,
	kMasterSum,
	kMasterInsert,
};

enum MasterFilterMode
{
	kFilterLowpass,
	kFilterHighpass,
};

constexpr int kParamFadeMs = 0;
constexpr int kParamRoutes = kParamFadeMs + 1;
constexpr int kParamMainL = kParamRoutes + kNumRoutes * kNumRouteParams;
constexpr int kParamMainR = kParamMainL + 1;
constexpr int kParamBypassL = kParamMainL + 2;
constexpr int kParamBypassR = kParamMainL + 3;
constexpr int kParamFx1L = kParamMainL + 4;
constexpr int kParamFx1R = kParamMainL + 5;
constexpr int kParamFx1Width = kParamMainL + 6;
constexpr int kParamFx1ReturnL = kParamMainL + 7;
constexpr int kParamFx1ReturnR = kParamMainL + 8;
constexpr int kParamFx1ReturnWidth = kParamMainL + 9;
constexpr int kParamFx1ReturnPath = kParamMainL + 10;
constexpr int kParamFx2L = kParamMainL + 11;
constexpr int kParamFx2R = kParamMainL + 12;
constexpr int kParamFx2Width = kParamMainL + 13;
constexpr int kParamFx2ReturnL = kParamMainL + 14;
constexpr int kParamFx2ReturnR = kParamMainL + 15;
constexpr int kParamFx2ReturnWidth = kParamMainL + 16;
constexpr int kParamFx2ReturnPath = kParamMainL + 17;
constexpr int kParamRepeatProtection = kParamFx2ReturnPath + 1;
constexpr int kParamSidechainMode = kParamRepeatProtection + 1;
constexpr int kParamSidechainKeyInput = kParamSidechainMode + 1;
constexpr int kParamSidechainDepth = kParamSidechainMode + 2;
constexpr int kParamSidechainLookahead = kParamSidechainMode + 3;
constexpr int kParamSidechainEnvLength = kParamSidechainMode + 4;
constexpr int kParamSidechainCurve = kParamSidechainMode + 5;
constexpr int kParamSidechainSmooth = kParamSidechainMode + 6;
constexpr int kParamMasterFilterEnable = kParamSidechainSmooth + 1;
constexpr int kParamMasterFilterHpCutoff = kParamMasterFilterEnable + 1;
constexpr int kParamMasterFilterLpCutoff = kParamMasterFilterEnable + 2;
constexpr int kParamMasterFilterQ = kParamMasterFilterEnable + 3;
constexpr int kParamMasterFilterSweep = kParamMasterFilterEnable + 4;
constexpr int kParamMasterMode = kParamMasterFilterSweep + 1;
constexpr int kParamMasterSendL = kParamMasterMode + 1;
constexpr int kParamMasterSendR = kParamMasterMode + 2;
constexpr int kParamMasterReturnL = kParamMasterMode + 3;
constexpr int kParamMasterReturnR = kParamMasterMode + 4;
constexpr int kParamMasterGain = kParamMasterReturnR + 1;
constexpr int kParamBypassOffset = kParamMasterGain + 1;
constexpr int kNumGlobalParams = kParamBypassOffset + 1;

enum ChannelParam
{
	kChannelEnable,
	kChannelInputL,
	kChannelInputR,
	kChannelGain,
	kChannelInsert1,
	kChannelInsert1Slot1,
	kChannelInsert1Slot2,
	kChannelInsert1Slot3,
	kChannelFx1Mix,
	kChannelInsert2,
	kChannelInsert2Slot1,
	kChannelInsert2Slot2,
	kChannelInsert2Slot3,
	kChannelOutputPath,
	kChannelFx2Mix,

	kNumChannelParams,
};

constexpr int kGlobalPageParams = 2;
constexpr int kRouteSetupParams = kParamMainL - 1;
constexpr int kFinalOutputParams = kParamFx1L - kParamMainL + 1; // Bypass Offset
constexpr int kFxSetupParams = kParamSidechainMode - kParamFx1L;
constexpr int kMasterPageParams = kNumGlobalParams - kParamSidechainMode - 1;
constexpr int kMaxParams = kNumGlobalParams + kMaxChannels * kNumChannelParams;
static_assert(kMaxParams == 237, "10-channel/8-route parameter budget changed");
static_assert(kMaxParams <= 241, "disting NT supports at most 241 parameters per algorithm");
static_assert(kNumGlobalParams == 87, "global parameter count changed");
static_assert(kParamMainL == 49, "final output page indices changed");
static_assert(kParamFx1L == 53, "FX setup page indices changed");
static_assert(kParamRepeatProtection == 67, "repeat protection index changed");
static_assert(kParamSidechainMode == 68, "master page indices changed");
static_assert(kNumChannelParams == 15, "channel parameter count changed");
static_assert(kChannelGain == 3, "Gain offset changed");
static_assert(kChannelInsert1 == 4, "Insert 1 offset changed");
static_assert(kChannelFx1Mix == 8, "FX Send 1 mix offset changed");
static_assert(kChannelInsert2 == 9, "Insert 2 offset changed");
static_assert(kChannelOutputPath == 13, "Output path offset changed");
static_assert(kChannelFx2Mix == 14, "FX Send 2 mix offset changed");
static_assert(kMaxParams <= 256, "parameter page indices are uint8_t");

static char const* const offOnStrings[] = {
	"Off", "On",
};

static char const* const outputPathStrings[] = {
	"Main", "Bypass",
};

static char const* const masterModeStrings[] = {
	"Split", "Sum", "Insert",
};

static char const* const widthStrings[] = {
	"Mono", "Stereo",
};

static char const* const channelPageNames[kMaxChannels] = {
	"Channel 1", "Channel 2", "Channel 3", "Channel 4",
	"Channel 5", "Channel 6", "Channel 7", "Channel 8",
	"Channel 9", "Channel 10",
};

static char const* const channelSuffixes[kNumChannelParams] = {
	"Enable",
	"Input/Left",
	"Right Input",
	"Gain",
	"Insert 1",
	"Insert 1 slot 1",
	"Insert 1 slot 2",
	"Insert 1 slot 3",
	"FX Send 1 mix",
	"Insert 2",
	"Insert 2 slot 1",
	"Insert 2 slot 2",
	"Insert 2 slot 3",
	"Output path",
	"FX Send 2 mix",
};

static char const* const defaultRouteNames[kNumRoutes] = {
	"Route A", "Route B", "Route C", "Route D",
	"Route E", "Route F", "Route G", "Route H",
};

static char const* const defaultFxNames[2] = {
	"FX Send 1", "FX Send 2",
};

static char const* const defaultSlotNames[kNumInserts][kNumInsertStates] = {
	{ "Dry", "Slot 1", "Slot 2", "Slot 3" },
	{ "Dry", "Slot 1", "Slot 2", "Slot 3" },
};

static char const* const insertStateStrings[] = {
	"Dry", "Slot 1", "Slot 2", "Slot 3", "Slot 3",
};

static char const* const defaultRouteParameterNames[kNumRoutes][kNumRouteParams] = {
	{
		"Route A output L", "Route A output R", "Route A return L",
		"Route A return R", "Route A send width", "Route A return width",
	},
	{
		"Route B output L", "Route B output R", "Route B return L",
		"Route B return R", "Route B send width", "Route B return width",
	},
	{
		"Route C output L", "Route C output R", "Route C return L",
		"Route C return R", "Route C send width", "Route C return width",
	},
	{
		"Route D output L", "Route D output R", "Route D return L",
		"Route D return R", "Route D send width", "Route D return width",
	},
	{
		"Route E output L", "Route E output R", "Route E return L",
		"Route E return R", "Route E send width", "Route E return width",
	},
	{
		"Route F output L", "Route F output R", "Route F return L",
		"Route F return R", "Route F send width", "Route F return width",
	},
	{
		"Route G output L", "Route G output R", "Route G return L",
		"Route G return R", "Route G send width", "Route G return width",
	},
	{
		"Route H output L", "Route H output R", "Route H return L",
		"Route H return R", "Route H send width", "Route H return width",
	},
};

static char const* const defaultFxParameterNames[2][7] = {
	{
		"FX Send 1 L", "FX Send 1 R", "FX Send 1 width",
		"FX 1 return L", "FX 1 return R", "FX 1 return width",
		"FX 1 return path",
	},
	{
		"FX Send 2 L", "FX Send 2 R", "FX Send 2 width",
		"FX 2 return L", "FX 2 return R", "FX 2 return width",
		"FX 2 return path",
	},
};

constexpr int routeParam(int route, int field)
{
	return kParamRoutes + route * kNumRouteParams + field;
}

static const uint8_t globalPageParams[kGlobalPageParams] = {
	kParamFadeMs, kParamRepeatProtection,
};

static const uint8_t routeSetupPageParams[kRouteSetupParams] = {
	routeParam(0, kRouteOutputL), routeParam(0, kRouteOutputR),
	routeParam(0, kRouteReturnL), routeParam(0, kRouteReturnR),
	routeParam(0, kRouteSendWidth), routeParam(0, kRouteReturnWidth),
	routeParam(1, kRouteOutputL), routeParam(1, kRouteOutputR),
	routeParam(1, kRouteReturnL), routeParam(1, kRouteReturnR),
	routeParam(1, kRouteSendWidth), routeParam(1, kRouteReturnWidth),
	routeParam(2, kRouteOutputL), routeParam(2, kRouteOutputR),
	routeParam(2, kRouteReturnL), routeParam(2, kRouteReturnR),
	routeParam(2, kRouteSendWidth), routeParam(2, kRouteReturnWidth),
	routeParam(3, kRouteOutputL), routeParam(3, kRouteOutputR),
	routeParam(3, kRouteReturnL), routeParam(3, kRouteReturnR),
	routeParam(3, kRouteSendWidth), routeParam(3, kRouteReturnWidth),
	routeParam(4, kRouteOutputL), routeParam(4, kRouteOutputR),
	routeParam(4, kRouteReturnL), routeParam(4, kRouteReturnR),
	routeParam(4, kRouteSendWidth), routeParam(4, kRouteReturnWidth),
	routeParam(5, kRouteOutputL), routeParam(5, kRouteOutputR),
	routeParam(5, kRouteReturnL), routeParam(5, kRouteReturnR),
	routeParam(5, kRouteSendWidth), routeParam(5, kRouteReturnWidth),
	routeParam(6, kRouteOutputL), routeParam(6, kRouteOutputR),
	routeParam(6, kRouteReturnL), routeParam(6, kRouteReturnR),
	routeParam(6, kRouteSendWidth), routeParam(6, kRouteReturnWidth),
	routeParam(7, kRouteOutputL), routeParam(7, kRouteOutputR),
	routeParam(7, kRouteReturnL), routeParam(7, kRouteReturnR),
	routeParam(7, kRouteSendWidth), routeParam(7, kRouteReturnWidth),
};

static const uint8_t finalOutputPageParams[kFinalOutputParams] = {
	kParamMainL, kParamMainR,
	kParamBypassL, kParamBypassR, kParamBypassOffset,
};

static const uint8_t fxSetupPageParams[kFxSetupParams] = {
	kParamFx1L, kParamFx1R,
	kParamFx1Width, kParamFx1ReturnL,
	kParamFx1ReturnR, kParamFx1ReturnWidth,
	kParamFx1ReturnPath, kParamFx2L,
	kParamFx2R, kParamFx2Width,
	kParamFx2ReturnL, kParamFx2ReturnR,
	kParamFx2ReturnWidth, kParamFx2ReturnPath,
};

static const uint8_t masterPageParams[kMasterPageParams] = {
	kParamSidechainMode, kParamSidechainKeyInput, kParamSidechainDepth,
	kParamSidechainLookahead, kParamSidechainEnvLength, kParamSidechainCurve,
	kParamSidechainSmooth,
	kParamMasterFilterEnable, kParamMasterFilterHpCutoff, kParamMasterFilterLpCutoff,
	kParamMasterFilterQ, kParamMasterFilterSweep, kParamMasterMode,
	kParamMasterSendL, kParamMasterSendR, kParamMasterReturnL, kParamMasterReturnR,
	kParamMasterGain,
};

static const _NT_specification specifications[] = {
	{ .name = "Channels", .min = 1, .max = kMaxChannels, .def = kMaxChannels, .type = kNT_typeGeneric },
};

struct SmoothedValue
{
	int parameterValue;
	int samplesRemaining;
	float target;
	float value;
	float increment;
};

struct ChannelRuntime
{
	bool initialised;
	int8_t insertState[kNumInserts];
	int insertSamplesRemaining[kNumInserts];
	float insertGain[kNumInserts][kNumInsertStates];
	float insertIncrement[kNumInserts][kNumInsertStates];
	SmoothedValue gain;
	SmoothedValue fx1Mix;
	SmoothedValue fx2Mix;
};

struct OutputPair
{
	float* left;
	float* right;
	bool singleFrame;
};

struct RampSmoothRuntime
{
	float value, target, increment;
	int remaining, samples;
};

struct SidechainRuntime
{
	bool initialised, keyHigh;
	int remaining;
	double value, increment, multiplier;
	float gain;
	RampSmoothRuntime smooth;
};

// Stereo rings belong to each instance's DRAM, independent of channel count.
constexpr int kMainDelayCapacity = 961;    // 10 ms at 96 kHz + current sample
constexpr int kBypassDelayCapacity = 9601; // 100 ms at 96 kHz + current sample
struct StereoDelay
{
	float* data;
	int capacity, write, current, target, requested, fadePosition, fadeSamples;
	bool initialised;
};

struct SvfRuntime
{
	float ic1eq;
	float ic2eq;
};

struct MasterFilterRuntime
{
	SvfRuntime hpLeft;
	SvfRuntime hpRight;
	SvfRuntime lpLeft;
	SvfRuntime lpRight;
};

typedef uint8_t ChannelPage[kNumChannelParams];

static _NT_parameter parameterDefs[kMaxParams];
static bool parameterTablesBuilt = false;

void buildParameters();

struct WitchboardAlgorithm : public _NT_algorithm
{
	WitchboardAlgorithm(int channels, uint8_t* dram);

	void buildPages();
	void setDefaultNames();

	int numChannels;
	_NT_parameterPages pages;
	_NT_parameterPage* pageDefs;
	ChannelPage* channelPages;
	ChannelRuntime* runtime;
	SidechainRuntime sidechain;
	MasterFilterRuntime masterFilter;
	StereoDelay mainDelay, bypassDelay;
	bool latencyInitialised;
	int previousAuto, previousEffective, manualTrim;
	bool savedTrim;
	int cachedLengthControl, cachedCurveControl, envSamples;
	float cachedSampleRate, curveBeta;
	SmoothedValue masterGain;
	bool masterGainInitialised;
	char channelNames[kMaxChannels][kHardwareNameLength];
	char routeNames[kNumRoutes][kHardwareNameLength];
	char fxNames[2][kHardwareNameLength];
	char slotNames[kNumInserts][kNumInsertStates][kSlotNameLength];
};

inline size_t alignedSize(size_t size, size_t alignment)
{
	return (size + alignment - 1) & ~(alignment - 1);
}

template <typename T>
size_t addStorage(size_t size, int count)
{
	return alignedSize(size, alignof(T)) + sizeof(T) * count;
}

template <typename T>
T* takeStorage(uint8_t*& cursor, int count)
{
	const uintptr_t address = alignedSize(reinterpret_cast<uintptr_t>(cursor), alignof(T));
	cursor = reinterpret_cast<uint8_t*>(address + sizeof(T) * count);
	return reinterpret_cast<T*>(address);
}

size_t requiredSram(int)
{
	return sizeof(WitchboardAlgorithm);
}

// Keep the algorithm and master DSP in SRAM. Allocate channel-dependent
// pages and routing state in the API's DRAM pool to reduce SRAM pressure.
size_t requiredDram(int channels)
{
	size_t size = 0;
	size = addStorage<_NT_parameterPage>(size, 5 + channels);
	size = addStorage<ChannelPage>(size, channels);
	size = addStorage<ChannelRuntime>(size, channels);
	size = addStorage<float>(size, 2 * (kMainDelayCapacity + kBypassDelayCapacity));
	return size;
}

WitchboardAlgorithm::WitchboardAlgorithm(int channels, uint8_t* dram)
	: numChannels(channels)
{
	pageDefs = takeStorage<_NT_parameterPage>(dram, 5 + numChannels);
	channelPages = takeStorage<ChannelPage>(dram, numChannels);
	runtime = takeStorage<ChannelRuntime>(dram, numChannels);
	memset(runtime, 0, sizeof(ChannelRuntime) * numChannels);
	memset(&sidechain, 0, sizeof(sidechain));
	memset(&masterFilter, 0, sizeof(masterFilter));
	memset(&mainDelay, 0, sizeof(mainDelay));
	memset(&bypassDelay, 0, sizeof(bypassDelay));
	mainDelay.capacity = kMainDelayCapacity;
	bypassDelay.capacity = kBypassDelayCapacity;
	mainDelay.data = takeStorage<float>(dram, 2 * kMainDelayCapacity);
	bypassDelay.data = takeStorage<float>(dram, 2 * kBypassDelayCapacity);
	memset(mainDelay.data, 0, sizeof(float) * 2 * kMainDelayCapacity);
	memset(bypassDelay.data, 0, sizeof(float) * 2 * kBypassDelayCapacity);
	latencyInitialised = false;
	previousAuto = previousEffective = manualTrim = 0;
	savedTrim = false;
	cachedLengthControl = cachedCurveControl = -10000;
	cachedSampleRate = curveBeta = 0;
	envSamples = 1;
	memset(&masterGain, 0, sizeof(masterGain));
	masterGainInitialised = false;
	setDefaultNames();
	if (!parameterTablesBuilt)
	{
		buildParameters();
		parameterTablesBuilt = true;
	}
	buildPages();
	parameters = parameterDefs;
	parameterPages = &pages;
}

inline int clampInt(int value, int minimum, int maximum)
{
	return value < minimum ? minimum : (value > maximum ? maximum : value);
}

inline int channelBase(int channel)
{
	return kNumGlobalParams + channel * kNumChannelParams;
}

inline int channelInsertSlotParam(int insert, int slot)
{
	return (insert == 0 ? kChannelInsert1Slot1 : kChannelInsert2Slot1) + slot;
}

inline int insertParameterToState(int value)
{
	return clampInt(value, 0, kNumInsertStates - 1);
}

void copyText(char* destination, int capacity, const char* source)
{
	if (!source)
		source = "";
	int i = 0;
	for (; i < capacity - 1 && source[i]; ++i)
		destination[i] = source[i];
	destination[i] = 0;
}

bool textEquals(const char* a, const char* b)
{
	if (!a)
		a = "";
	if (!b)
		b = "";
	while (*a && *b)
	{
		if (*a != *b)
			return false;
		++a;
		++b;
	}
	return *a == *b;
}

bool isDefaultChannelName(const WitchboardAlgorithm* self, int channel)
{
	return textEquals(self->channelNames[channel], channelPageNames[channel]);
}

bool isAutoSlotName(const char* name, int insert, int state)
{
	return state != kInsertDry
		&& (!name || !name[0] || textEquals(name, defaultSlotNames[insert][state]));
}

void setParameter(_NT_parameter& parameter, const char* name, int minimum, int maximum,
	int defaultValue, uint8_t unit, char const* const* strings = NULL)
{
	parameter.name = name;
	parameter.min = minimum;
	parameter.max = maximum;
	parameter.def = defaultValue;
	parameter.unit = unit;
	parameter.scaling = 0;
	parameter.enumStrings = strings;
}

void setInput(_NT_parameter& parameter, const char* name)
{
	setParameter(parameter, name, 0, kNT_lastBus, 0, kNT_unitAudioInput);
}

void setOutput(_NT_parameter& parameter, const char* name)
{
	setParameter(parameter, name, 0, kNT_lastBus, 0, kNT_unitAudioOutput);
}

void setWidth(_NT_parameter& parameter, const char* name, int defaultValue)
{
	setParameter(parameter, name, kWidthMono, kWidthStereo, defaultValue,
		kNT_unitEnum, widthStrings);
}

void WitchboardAlgorithm::setDefaultNames()
{
	for (int channel = 0; channel < kMaxChannels; ++channel)
		copyText(channelNames[channel], kHardwareNameLength, channelPageNames[channel]);
	for (int route = 0; route < kNumRoutes; ++route)
		copyText(routeNames[route], kHardwareNameLength, defaultRouteNames[route]);
	for (int fx = 0; fx < 2; ++fx)
		copyText(fxNames[fx], kHardwareNameLength, defaultFxNames[fx]);
	for (int insert = 0; insert < kNumInserts; ++insert)
	{
		copyText(slotNames[insert][kInsertDry], kSlotNameLength,
			defaultSlotNames[insert][kInsertDry]);
		for (int state = 0; state < kNumInsertStates; ++state)
			if (state != kInsertDry)
				copyText(slotNames[insert][state], kSlotNameLength, "");
	}
}

void buildParameters()
{
	setParameter(parameterDefs[kParamFadeMs], "Switch fade", 0, 100, 2, kNT_unitMs);

	for (int route = 0; route < kNumRoutes; ++route)
	{
		setOutput(parameterDefs[routeParam(route, kRouteOutputL)],
			defaultRouteParameterNames[route][kRouteOutputL]);
		setOutput(parameterDefs[routeParam(route, kRouteOutputR)],
			defaultRouteParameterNames[route][kRouteOutputR]);
		setInput(parameterDefs[routeParam(route, kRouteReturnL)],
			defaultRouteParameterNames[route][kRouteReturnL]);
		setInput(parameterDefs[routeParam(route, kRouteReturnR)],
			defaultRouteParameterNames[route][kRouteReturnR]);
		setWidth(parameterDefs[routeParam(route, kRouteSendWidth)],
			defaultRouteParameterNames[route][kRouteSendWidth], kWidthMono);
		setWidth(parameterDefs[routeParam(route, kRouteReturnWidth)],
			defaultRouteParameterNames[route][kRouteReturnWidth], kWidthMono);
	}

	setOutput(parameterDefs[kParamMainL], "Main L");
	setOutput(parameterDefs[kParamMainR], "Main R");
	setOutput(parameterDefs[kParamBypassL], "Bypass L");
	setOutput(parameterDefs[kParamBypassR], "Bypass R");

	setOutput(parameterDefs[kParamFx1L], defaultFxParameterNames[0][0]);
	setOutput(parameterDefs[kParamFx1R], defaultFxParameterNames[0][1]);
	setWidth(parameterDefs[kParamFx1Width], defaultFxParameterNames[0][2], kWidthStereo);
	setInput(parameterDefs[kParamFx1ReturnL], defaultFxParameterNames[0][3]);
	setInput(parameterDefs[kParamFx1ReturnR], defaultFxParameterNames[0][4]);
	setWidth(parameterDefs[kParamFx1ReturnWidth], defaultFxParameterNames[0][5],
		kWidthStereo);
	setParameter(parameterDefs[kParamFx1ReturnPath], defaultFxParameterNames[0][6], 0, 1,
		kOutputPathMain, kNT_unitEnum, outputPathStrings);

	setOutput(parameterDefs[kParamFx2L], defaultFxParameterNames[1][0]);
	setOutput(parameterDefs[kParamFx2R], defaultFxParameterNames[1][1]);
	setWidth(parameterDefs[kParamFx2Width], defaultFxParameterNames[1][2], kWidthStereo);
	setInput(parameterDefs[kParamFx2ReturnL], defaultFxParameterNames[1][3]);
	setInput(parameterDefs[kParamFx2ReturnR], defaultFxParameterNames[1][4]);
	setWidth(parameterDefs[kParamFx2ReturnWidth], defaultFxParameterNames[1][5],
		kWidthStereo);
	setParameter(parameterDefs[kParamFx2ReturnPath], defaultFxParameterNames[1][6], 0, 1,
		kOutputPathMain, kNT_unitEnum, outputPathStrings);

	setParameter(parameterDefs[kParamRepeatProtection], "Repeat protection", 0, 1, 1,
		kNT_unitEnum, offOnStrings);

	setParameter(parameterDefs[kParamSidechainMode], "Sidechain", 0, 1, 0,
		kNT_unitEnum, offOnStrings);
	setInput(parameterDefs[kParamSidechainKeyInput], "SC Trigger Input");
	parameterDefs[kParamSidechainKeyInput].unit = kNT_unitCvInput;
	setParameter(parameterDefs[kParamSidechainDepth], "SC Depth", 0, 100, 69, kNT_unitPercent);
	setParameter(parameterDefs[kParamSidechainLookahead], "SC Lookahead", 0, 100, 60, kNT_unitMs);
	parameterDefs[kParamSidechainLookahead].scaling = kNT_scaling10;
	// Normalized log control, displayed in actual milliseconds by parameterString.
	setParameter(parameterDefs[kParamSidechainEnvLength], "SC Env Length", 0, 1000, 486, kNT_unitHasStrings);
	setParameter(parameterDefs[kParamSidechainCurve], "SC Curve", -100, 100, 20, kNT_unitNone);
	setParameter(parameterDefs[kParamSidechainSmooth], "SC Smooth", 0, 100, 4, kNT_unitPercent);
	setParameter(parameterDefs[kParamBypassOffset], "Bypass Offset", 0, 1000, 0, kNT_unitMs);
	parameterDefs[kParamBypassOffset].scaling = kNT_scaling10;

	setParameter(parameterDefs[kParamMasterFilterEnable], "Filter enable", 0, 1, 0,
		kNT_unitEnum, offOnStrings);
	setParameter(parameterDefs[kParamMasterFilterHpCutoff], "HP limit", 0, 100, 70,
		kNT_unitPercent);
	setParameter(parameterDefs[kParamMasterFilterLpCutoff], "LP limit", 0, 100, 20,
		kNT_unitPercent);
	setParameter(parameterDefs[kParamMasterFilterQ], "Filter Q", 0, 100, 10,
		kNT_unitPercent);
	setParameter(parameterDefs[kParamMasterFilterSweep], "Filter sweep", -100, 100, 0,
		kNT_unitPercent);

	setParameter(parameterDefs[kParamMasterMode], "Master output", 0, 2, 0,
		kNT_unitEnum, masterModeStrings);
	setOutput(parameterDefs[kParamMasterSendL], "Master send L");
	setOutput(parameterDefs[kParamMasterSendR], "Master send R");
	setInput(parameterDefs[kParamMasterReturnL], "Master return L");
	setInput(parameterDefs[kParamMasterReturnR], "Master return R");

	setParameter(parameterDefs[kParamMasterGain], "Master Gain", -12, 6, 0, kNT_unitDb);

	for (int channel = 0; channel < kMaxChannels; ++channel)
	{
		const int base = channelBase(channel);
		setParameter(parameterDefs[base + kChannelEnable],
			channelSuffixes[kChannelEnable], 0, 1, 1, kNT_unitEnum, offOnStrings);
		setInput(parameterDefs[base + kChannelInputL], channelSuffixes[kChannelInputL]);
		setInput(parameterDefs[base + kChannelInputR], channelSuffixes[kChannelInputR]);
		setParameter(parameterDefs[base + kChannelGain],
			channelSuffixes[kChannelGain], -60, 6, 0, kNT_unitDb_minInf);
		setParameter(parameterDefs[base + kChannelInsert1],
			channelSuffixes[kChannelInsert1], 0, kInsertParameterMax, 0,
			kNT_unitHasStrings);
		for (int slot = 0; slot < 3; ++slot)
			setParameter(parameterDefs[base + kChannelInsert1Slot1 + slot],
				channelSuffixes[kChannelInsert1Slot1 + slot], 0, kNumRoutes - 1,
				slot, kNT_unitHasStrings);
		setParameter(parameterDefs[base + kChannelFx1Mix],
			channelSuffixes[kChannelFx1Mix], 0, 100, 0, kNT_unitPercent);
		setParameter(parameterDefs[base + kChannelInsert2],
			channelSuffixes[kChannelInsert2], 0, kInsertParameterMax, 0,
			kNT_unitHasStrings);
		for (int slot = 0; slot < 3; ++slot)
			setParameter(parameterDefs[base + kChannelInsert2Slot1 + slot],
				channelSuffixes[kChannelInsert2Slot1 + slot], 0, kNumRoutes - 1,
				slot, kNT_unitHasStrings);
		setParameter(parameterDefs[base + kChannelOutputPath],
			channelSuffixes[kChannelOutputPath], 0, 1, kOutputPathMain,
			kNT_unitEnum, outputPathStrings);
		setParameter(parameterDefs[base + kChannelFx2Mix],
			channelSuffixes[kChannelFx2Mix], 0, 100, 0, kNT_unitPercent);
	}
}

void WitchboardAlgorithm::buildPages()
{
	int page = 0;
	pageDefs[page++] = {
		.name = "Global",
		.numParams = kGlobalPageParams,
		.group = 1,
		.unused = { 0, 0 },
		.params = globalPageParams,
	};

	pageDefs[page++] = {
		.name = "Route Setup",
		.numParams = kRouteSetupParams,
		.group = 2,
		.unused = { 0, 0 },
		.params = routeSetupPageParams,
	};

	pageDefs[page++] = {
		.name = "Final Outputs",
		.numParams = kFinalOutputParams,
		.group = 3,
		.unused = { 0, 0 },
		.params = finalOutputPageParams,
	};

	pageDefs[page++] = {
		.name = "FX Setup",
		.numParams = kFxSetupParams,
		.group = 4,
		.unused = { 0, 0 },
		.params = fxSetupPageParams,
	};

	pageDefs[page++] = {
		.name = "Sidechain/Master",
		.numParams = kMasterPageParams,
		.group = 5,
		.unused = { 0, 0 },
		.params = masterPageParams,
	};

	for (int channel = 0; channel < numChannels; ++channel)
	{
		const int base = channelBase(channel);
		for (int i = 0; i < kNumChannelParams; ++i)
			channelPages[channel][i] = base + i;
		pageDefs[page++] = {
			.name = channelNames[channel],
			.numParams = kNumChannelParams,
			.group = static_cast<uint8_t>(6 + channel),
			.unused = { 0, 0 },
			.params = channelPages[channel],
		};
	}

	pages.numPages = page;
	pages.pages = pageDefs;
}

inline const float* inputBus(const float* busFrames, int bus, int numFrames)
{
	return bus > 0 && bus <= kNT_lastBus ? busFrames + (bus - 1) * numFrames : NULL;
}

inline float* outputBus(float* busFrames, int bus, int numFrames)
{
	return bus > 0 && bus <= kNT_lastBus ? busFrames + (bus - 1) * numFrames : NULL;
}

inline float dbGain(int decibels)
{
	return decibels <= -60 ? 0.0f : powf(10.0f, decibels / 20.0f);
}

inline float percentGain(int percent)
{
	return clampInt(percent, 0, 100) * 0.01f;
}

inline float clampFloat(float value, float minimum, float maximum)
{
	return value < minimum ? minimum : (value > maximum ? maximum : value);
}

constexpr float kPi = 3.14159265358979323846f;

float envLengthFromNormalized(float x)
{
	return 50.0f * powf(40.0f, clampFloat(x, 0.0f, 1.0f));
}

int millisecondsToSamples(float ms, float sampleRate)
{
	return static_cast<int>(ms * sampleRate * 0.001f + 0.5f);
}

float curveToBeta(float c)
{
	c = clampFloat(c, -1.0f, 1.0f);
	if (fabsf(c) < 0.001f) return 0.0f;
	float hp = powf((fabsf(c) + 1.0e-20f) * 1.2f, 0.41f) * 0.91f;
	hp = clampFloat(hp, 0.0f, 0.999999f);
	const float fp = hp / (1.0f - hp);
	return c < 0.0f ? -fp : fp;
}

void startSidechainEnvelope(SidechainRuntime& sc, int lengthSamples, float beta)
{
	sc.remaining = lengthSamples < 1 ? 1 : lengthSamples;
	sc.value = 0.0;
	sc.multiplier = 1.0;
	sc.increment = 1.0 / sc.remaining;
	if (beta != 0.0f)
	{
		// expm1(beta/N), without small-argument subtraction or an extra libm import.
		// |z| < 0.031 for the public ranges at all supported sample rates.
		const double z = static_cast<double>(beta) / sc.remaining;
		const double qm1 = z * (1 + z * (0.5 + z * (1.0/6 + z * (1.0/24
			+ z * (1.0/120 + z / 720)))));
		sc.multiplier = 1.0 + qm1;
		sc.increment = qm1 / (static_cast<double>(powf(2.718281828459045f, beta)) - 1.0);
	}
}

float rampSmooth(RampSmoothRuntime& ramp, float input, int samples)
{
	if (samples <= 1)
	{
		ramp.value = ramp.target = input;
		ramp.remaining = 0;
		ramp.samples = samples;
		return input;
	}
	// rampsmooth~ semantics: each changed input starts a new linear ramp.
	// A constant input must finish in N samples, rather than decay exponentially.
	if (input != ramp.target || samples != ramp.samples)
	{
		ramp.target = input;
		ramp.samples = samples;
		ramp.remaining = samples;
		ramp.increment = (input - ramp.value) / samples;
	}
	if (ramp.remaining > 0)
	{
		ramp.value += ramp.increment;
		if (--ramp.remaining == 0) ramp.value = ramp.target;
	}
	return clampFloat(ramp.value, 0.0f, 1.0f);
}

float processSidechain(SidechainRuntime& sc, float key, int lengthSamples,
	float beta, int smoothSamples, float depth)
{
	if (!sc.initialised)
	{
		sc.value = sc.gain = sc.smooth.value = sc.smooth.target = 1.0f;
		sc.initialised = true;
	}
	const bool high = fabsf(key) > 0.1f;
	if (high && !sc.keyHigh)
		startSidechainEnvelope(sc, lengthSamples, beta);
	else if (sc.remaining > 0)
	{
		sc.value += sc.increment;
		sc.increment *= sc.multiplier;
		if (--sc.remaining == 0) sc.value = 1.0;
	}
	sc.keyHigh = high;
	const float envelope = rampSmooth(sc.smooth,
		clampFloat(static_cast<float>(sc.value), 0.0f, 1.0f), smoothSamples);
	sc.gain = 1.0f - clampFloat(depth, 0.0f, 1.0f) * (1.0f - envelope);
	return sc.gain;
}

void setDelay(StereoDelay& delay, int samples, int fadeSamples)
{
	delay.requested = clampInt(samples, 0, delay.capacity - 1);
	delay.fadeSamples = fadeSamples < 1 ? 1 : fadeSamples;
	if (!delay.initialised)
	{
		delay.current = delay.target = delay.requested;
		delay.initialised = true;
	}
}

void processDelay(StereoDelay& delay, float& left, float& right)
{
	delay.data[2 * delay.write] = left;
	delay.data[2 * delay.write + 1] = right;
	// Finish a fade before adopting the latest request. Never reset an audible
	// crossfade mid-flight, even if CV supplies a different delay every block.
	if (delay.fadePosition == 0 && delay.current != delay.requested)
		delay.target = delay.requested;
	int oldRead = delay.write - delay.current;
	if (oldRead < 0) oldRead += delay.capacity;
	left = delay.data[2 * oldRead];
	right = delay.data[2 * oldRead + 1];
	if (delay.current != delay.target)
	{
		int newRead = delay.write - delay.target;
		if (newRead < 0) newRead += delay.capacity;
		const float mix = static_cast<float>(++delay.fadePosition) / delay.fadeSamples;
		left += mix * (delay.data[2 * newRead] - left);
		right += mix * (delay.data[2 * newRead + 1] - right);
		if (delay.fadePosition >= delay.fadeSamples)
		{
			delay.current = delay.target;
			delay.fadePosition = 0;
		}
	}
	if (++delay.write == delay.capacity) delay.write = 0;
}

int followBypassOffset(WitchboardAlgorithm* self)
{
	const int activeAuto = self->v[kParamSidechainMode]
		? clampInt(self->v[kParamSidechainLookahead], 0, 100) : 0;
	const int publicValue = clampInt(self->v[kParamBypassOffset], 0, 1000);
	// First audio block after loading: saved effective delay is authoritative.
	// Optional trim metadata preserves intent when physical delay was clamped.
	if (!self->latencyInitialised)
	{
		if (!self->savedTrim || clampInt(self->manualTrim + activeAuto, 0, 1000) != publicValue)
			self->manualTrim = publicValue - activeAuto;
		self->savedTrim = false;
	}
	else if (publicValue != self->previousEffective)
		self->manualTrim = publicValue - self->previousAuto;
	const int effective = clampInt(self->manualTrim + activeAuto, 0, 1000);
	// Update state before the API setter, which may synchronously notify us.
	self->previousAuto = activeAuto;
	self->previousEffective = effective;
	self->latencyInitialised = true;
	if (effective != publicValue)
	{
		const int index = NT_algorithmIndex(self);
		if (index >= 0)
			NT_setParameterFromAudio(index, kParamBypassOffset + NT_parameterOffset(), effective);
	}
	return effective;
}

// Small decimal formatter avoids introducing printf into the NT object.
int numberString(char* buffer, unsigned value, int decimals, const char* suffix)
{
	char digits[12];
	int count = 0;
	do { digits[count++] = static_cast<char>('0' + value % 10); value /= 10; }
	while (value || count <= decimals);
	int length = 0;
	while (count)
	{
		buffer[length++] = digits[--count];
		if (decimals && count == decimals) buffer[length++] = '.';
	}
	while (*suffix) buffer[length++] = *suffix++;
	buffer[length] = 0;
	return length;
}

struct SvfCoefficients
{
	float a1;
	float a2;
	float a3;
	float k;
	float m0;
	float m1;
	float m2;
};

float hpCutoffPercentToHz(int cutoffPercent, float sampleRate)
{
	const float percent = clampFloat(cutoffPercent * 0.01f, 0.0f, 1.0f);
	const float frequency = 20.0f * powf(1000.0f, percent);
	const float maximum = sampleRate * 0.45f;
	return clampFloat(frequency, 20.0f, maximum);
}

float filterQFromPercent(int qPercent)
{
	return 0.70710678f + clampFloat(qPercent * 0.01f, 0.0f, 1.0f) * 11.29289322f;
}

float filterTan(float radians)
{
	radians = clampFloat(radians, 0.0f, kPi * 0.45f);
	const float x2 = radians * radians;
	const float pi2 = kPi * kPi;
	const float denominator = pi2 - 4.0f * x2;
	if (denominator <= 0.001f)
		return 64.0f;
	return radians * (pi2 - x2) / denominator;
}

SvfCoefficients makeFilterCoefficients(int mode, float frequency, float q)
{
	const float configuredSampleRate = NT_globals.sampleRate;
	const float sampleRate = configuredSampleRate > 0.0f ? configuredSampleRate : 48000.0f;
	const float g = filterTan(kPi * frequency / sampleRate);
	const float k = 1.0f / q;
	SvfCoefficients coeffs = {};

	coeffs.a1 = 1.0f / (1.0f + g * (g + k));
	coeffs.a2 = g * coeffs.a1;
	coeffs.a3 = g * coeffs.a2;
	coeffs.k = k;

	switch (mode)
	{
	case kFilterHighpass:
		coeffs.m0 = 1.0f;
		coeffs.m1 = -k;
		coeffs.m2 = -1.0f;
		break;
	default:
		coeffs.m0 = 0.0f;
		coeffs.m1 = 0.0f;
		coeffs.m2 = 1.0f;
		break;
	}
	return coeffs;
}

float processFilterSample(SvfRuntime& state, float input,
	const SvfCoefficients& coeffs)
{
	float v0 = input;
	const float v3 = v0 - state.ic2eq;
	const float v1 = coeffs.a1 * state.ic1eq + coeffs.a2 * v3;
	const float v2 = state.ic2eq + coeffs.a2 * state.ic1eq + coeffs.a3 * v3;
	state.ic1eq = 2.0f * v1 - state.ic1eq;
	state.ic2eq = 2.0f * v2 - state.ic2eq;
	return coeffs.m0 * v0 + coeffs.m1 * v1 + coeffs.m2 * v2;
}

void processMasterFilter(MasterFilterRuntime& filter, float& left, float& right,
	bool lpEnabled, bool hpEnabled,
	const SvfCoefficients& lpCoeffs, const SvfCoefficients& hpCoeffs)
{
	if (lpEnabled)
	{
		left = processFilterSample(filter.lpLeft, left, lpCoeffs);
		right = processFilterSample(filter.lpRight, right, lpCoeffs);
	}
	else if (hpEnabled)
	{
		left = processFilterSample(filter.hpLeft, left, hpCoeffs);
		right = processFilterSample(filter.hpRight, right, hpCoeffs);
	}
}

void resetMasterFilterAudioState(MasterFilterRuntime& filter)
{
	memset(&filter.hpLeft, 0, sizeof(filter.hpLeft));
	memset(&filter.hpRight, 0, sizeof(filter.hpRight));
	memset(&filter.lpLeft, 0, sizeof(filter.lpLeft));
	memset(&filter.lpRight, 0, sizeof(filter.lpRight));
}

void initialiseSmooth(SmoothedValue& smooth, int parameterValue, float value)
{
	smooth.parameterValue = parameterValue;
	smooth.samplesRemaining = 0;
	smooth.target = value;
	smooth.value = value;
	smooth.increment = 0.0f;
}

void beginSmooth(SmoothedValue& smooth, int parameterValue, float target, int fadeSamples)
{
	smooth.parameterValue = parameterValue;
	smooth.target = target;
	if (fadeSamples <= 0)
	{
		smooth.samplesRemaining = 0;
		smooth.value = target;
		smooth.increment = 0.0f;
		return;
	}
	smooth.samplesRemaining = fadeSamples;
	smooth.increment = (target - smooth.value) / fadeSamples;
}

void advanceSmooth(SmoothedValue& smooth)
{
	if (smooth.samplesRemaining <= 0)
		return;
	smooth.value += smooth.increment;
	if (--smooth.samplesRemaining == 0)
		smooth.value = smooth.target;
}

void initialiseChannel(ChannelRuntime& runtime, int insert1, int insert2,
	int gainDb, int fx1Percent, int fx2Percent)
{
	runtime.initialised = true;
	const int inserts[kNumInserts] = { insert1, insert2 };
	for (int insert = 0; insert < kNumInserts; ++insert)
	{
		runtime.insertState[insert] = inserts[insert];
		runtime.insertSamplesRemaining[insert] = 0;
		for (int state = 0; state < kNumInsertStates; ++state)
		{
			runtime.insertGain[insert][state] = state == inserts[insert] ? 1.0f : 0.0f;
			runtime.insertIncrement[insert][state] = 0.0f;
		}
	}
	initialiseSmooth(runtime.gain, gainDb, dbGain(gainDb));
	initialiseSmooth(runtime.fx1Mix, fx1Percent, percentGain(fx1Percent));
	initialiseSmooth(runtime.fx2Mix, fx2Percent, percentGain(fx2Percent));
}

void beginInsertFade(ChannelRuntime& runtime, int insert, int state, int fadeSamples)
{
	runtime.insertState[insert] = state;
	if (fadeSamples <= 0)
	{
		runtime.insertSamplesRemaining[insert] = 0;
		for (int i = 0; i < kNumInsertStates; ++i)
		{
			runtime.insertGain[insert][i] = i == state ? 1.0f : 0.0f;
			runtime.insertIncrement[insert][i] = 0.0f;
		}
		return;
	}

	runtime.insertSamplesRemaining[insert] = fadeSamples;
	for (int i = 0; i < kNumInsertStates; ++i)
	{
		const float target = i == state ? 1.0f : 0.0f;
		runtime.insertIncrement[insert][i] =
			(target - runtime.insertGain[insert][i]) / fadeSamples;
	}
}

void advanceChannel(ChannelRuntime& runtime)
{
	for (int insert = 0; insert < kNumInserts; ++insert)
	{
		if (runtime.insertSamplesRemaining[insert] <= 0)
			continue;
		for (int state = 0; state < kNumInsertStates; ++state)
			runtime.insertGain[insert][state] += runtime.insertIncrement[insert][state];
		if (--runtime.insertSamplesRemaining[insert] == 0)
		{
			for (int state = 0; state < kNumInsertStates; ++state)
				runtime.insertGain[insert][state] =
					state == runtime.insertState[insert] ? 1.0f : 0.0f;
		}
	}
	advanceSmooth(runtime.gain);
	advanceSmooth(runtime.fx1Mix);
	advanceSmooth(runtime.fx2Mix);
}

// NT parameter changes can arrive outside the audio step. Invalidate only the
// affected insert cache so the existing step() code observes the current self->v
// value and starts the normal insert fade on the next audio block.
void parameterChanged(_NT_algorithm* algorithm, int parameter)
{
	WitchboardAlgorithm* self = static_cast<WitchboardAlgorithm*>(algorithm);
	if (!self)
		return;

	if (parameter < kNumGlobalParams)
		return;

	const int relative = parameter - kNumGlobalParams;
	const int channel = relative / kNumChannelParams;
	const int field = relative % kNumChannelParams;
	if (channel < 0 || channel >= self->numChannels)
		return;

	int insert = -1;
	if (field == kChannelInsert1)
		insert = 0;
	else if (field == kChannelInsert2)
		insert = 1;
	else
		return;

	ChannelRuntime& rt = self->runtime[channel];

	// Valid insert states are 0..3, so -1 is guaranteed to differ from the
	// newly selected state. step() will call beginInsertFade() using self->v.
	rt.insertState[insert] = -1;
}

OutputPair makeOutputPair(float* busFrames, int numFrames, int leftBus, int rightBus)
{
	OutputPair pair;
	pair.left = outputBus(busFrames, leftBus, numFrames);
	pair.right = outputBus(busFrames, rightBus, numFrames);
	pair.singleFrame = false;
	return pair;
}

inline void addSignal(const OutputPair& pair, int frame, float left, float right,
	bool stereo, float gain)
{
	if (gain <= 0.0f || !pair.left)
		return;
	const int index = pair.singleFrame ? 0 : frame;
	if (pair.right)
	{
		pair.left[index] += left * gain;
		pair.right[index] += (stereo ? right : left) * gain;
	}
	else
	{
		const float mono = stereo ? 0.5f * (left + right) : left;
		pair.left[index] += mono * gain;
	}
}

struct CrossfadeGains
{
	float dry;
	float wet;
};

inline CrossfadeGains shapedCrossfade(float mix)
{
	mix = mix < 0.0f ? 0.0f : (mix > 1.0f ? 1.0f : mix);
	if (mix <= 0.5f)
		return { 1.0f, mix * 2.0f };
	return { (1.0f - mix) * 2.0f, 1.0f };
}

inline void processPath(OutputPair* outputs,
	const float* const* returnLeft, const float* const* returnRight,
	const bool* returnStereo, int frame, float left, float right, bool stereo,
	int route1, int route2, bool repeatProtection, int outputIndex,
	float pathGain, float channelGain, const CrossfadeGains& fx1,
	const CrossfadeGains& fx2)
{
	if (pathGain <= 0.0f)
		return;

	left *= channelGain;
	right *= channelGain;

	float intermediateLeft = left;
	float intermediateRight = right;
	bool intermediateStereo = stereo;
	if (route1 >= 0)
	{
		addSignal(outputs[4 + route1], frame, left, right, stereo, pathGain);
		intermediateLeft = returnLeft[route1] ? returnLeft[route1][frame] : 0.0f;
		intermediateRight = returnRight[route1] ? returnRight[route1][frame] : intermediateLeft;
		intermediateStereo = returnStereo[route1];
	}
	if (repeatProtection && route1 >= 0 && route2 == route1)
		route2 = -1;

	float finalLeft = intermediateLeft;
	float finalRight = intermediateRight;
	bool finalStereo = intermediateStereo;
	if (route2 >= 0)
	{
		addSignal(outputs[4 + route2], frame, intermediateLeft, intermediateRight,
			intermediateStereo, pathGain);
		finalLeft = returnLeft[route2] ? returnLeft[route2][frame] : 0.0f;
		finalRight = returnRight[route2] ? returnRight[route2][frame] : finalLeft;
		finalStereo = returnStereo[route2];
	}

	const float dryMix = fx1.dry * fx2.dry;
	addSignal(outputs[outputIndex], frame, finalLeft, finalRight, finalStereo,
		pathGain * dryMix);
	addSignal(outputs[2], frame, finalLeft, finalRight, finalStereo,
		pathGain * fx1.wet);
	addSignal(outputs[3], frame, finalLeft, finalRight, finalStereo,
		pathGain * fx2.wet);
}

struct ChannelBlockState
{
	int base;
	const float* left;
	const float* right;
	bool enabled;
	bool stereo;
	bool moving;
	int outputIndex;
	int8_t routes[kNumInserts][kNumInsertStates];
	CrossfadeGains fxGains[2];
};

int selectedRoute(const WitchboardAlgorithm* self, int channel, int insert, int state)
{
	if (state == kInsertDry)
		return -1;
	const int base = channelBase(channel);
	const int slot = clampInt(state, 1, 3) - 1;
	const int param = base + channelInsertSlotParam(insert, slot);
	return clampInt(self->v[param], 0, kNumRoutes - 1);
}

_NT_DRAM_SECTION
void calculateRequirements(_NT_algorithmRequirements& requirements, const int32_t* specs)
{
	const int channels = clampInt(specs[0], 1, kMaxChannels);
	requirements.numParameters = kNumGlobalParams + channels * kNumChannelParams;
	requirements.sram = static_cast<uint32_t>(requiredSram(channels));
	requirements.dram = static_cast<uint32_t>(requiredDram(channels));
	requirements.dtc = 0;
	requirements.itc = 0;
}

_NT_DRAM_SECTION
_NT_algorithm* constructWitchboard(const _NT_algorithmMemoryPtrs& pointers,
	const _NT_algorithmRequirements&, const int32_t* specs)
{
	const int channels = clampInt(specs[0], 1, kMaxChannels);
	return new (pointers.sram) WitchboardAlgorithm(channels, pointers.dram);
}

void step(_NT_algorithm* algorithm, float* busFrames, int numFramesBy4)
{
	WitchboardAlgorithm* self = static_cast<WitchboardAlgorithm*>(algorithm);
	const int numFrames = numFramesBy4 * 4;
	const int fadeSamples = self->v[kParamFadeMs]
		* static_cast<int>(NT_globals.sampleRate) / 1000;

	OutputPair outputs[kNumOutputPairs];
	const OutputPair mainOutput = makeOutputPair(busFrames, numFrames, self->v[kParamMainL],
		self->v[kParamMainR]);
	const OutputPair bypassOutput = makeOutputPair(busFrames, numFrames, self->v[kParamBypassL],
		self->v[kParamBypassR]);
	outputs[0] = mainOutput;
	outputs[1] = bypassOutput;
	outputs[2] = makeOutputPair(busFrames, numFrames, self->v[kParamFx1L],
		self->v[kParamFx1Width] == kWidthStereo ? self->v[kParamFx1R] : 0);
	outputs[3] = makeOutputPair(busFrames, numFrames, self->v[kParamFx2L],
		self->v[kParamFx2Width] == kWidthStereo ? self->v[kParamFx2R] : 0);
	for (int route = 0; route < kNumRoutes; ++route)
	{
		outputs[4 + route] = makeOutputPair(busFrames, numFrames,
			self->v[routeParam(route, kRouteOutputL)],
			self->v[routeParam(route, kRouteSendWidth)] == kWidthStereo
				? self->v[routeParam(route, kRouteOutputR)] : 0);
	}

	const float* returnLeft[kNumRoutes];
	const float* returnRight[kNumRoutes];
	bool returnStereo[kNumRoutes];
	for (int route = 0; route < kNumRoutes; ++route)
	{
		returnLeft[route] = inputBus(busFrames,
			self->v[routeParam(route, kRouteReturnL)], numFrames);
		returnRight[route] = self->v[routeParam(route, kRouteReturnWidth)] == kWidthStereo
			? inputBus(busFrames, self->v[routeParam(route, kRouteReturnR)], numFrames) : NULL;
		returnStereo[route] = returnRight[route] != NULL;
	}

	const bool sidechainEnabled = self->v[kParamSidechainMode] != 0;
	const int masterMode = clampInt(self->v[kParamMasterMode], kMasterSplit, kMasterInsert);
	const bool filterOn = self->v[kParamMasterFilterEnable] != 0;
	const int rawSweep = clampInt(self->v[kParamMasterFilterSweep], -100, 100);
	const int filterSweep = filterOn ? rawSweep : 0;
	const int hpLimit = clampInt(self->v[kParamMasterFilterHpCutoff], 0, 100);
	const int lpLimit = clampInt(self->v[kParamMasterFilterLpCutoff], 0, 100);
	const bool lpSweep = filterSweep < 0;
	const bool hpSweep = filterSweep > 0;
	int lpCutoff = 0;
	if (lpSweep)
		lpCutoff = 100 - (100 - lpLimit) * -filterSweep / 100;
	int hpCutoff = 0;
	if (hpSweep)
		hpCutoff = hpLimit * filterSweep / 100;
	const bool lpEnabled = filterOn && lpSweep;
	const bool hpEnabled = filterOn && hpSweep;
	const bool anyFilterEnabled = lpEnabled || hpEnabled;
	const bool repeatProtection = self->v[kParamRepeatProtection] != 0;
	const float gainSampleRate = NT_globals.sampleRate > 0 ? NT_globals.sampleRate : 48000;
	const int masterGainDb = clampInt(self->v[kParamMasterGain], -12, 6);
	if (!self->masterGainInitialised)
	{
		initialiseSmooth(self->masterGain, masterGainDb, dbGain(masterGainDb));
		self->masterGainInitialised = true;
	}
	else if (self->masterGain.parameterValue != masterGainDb)
		beginSmooth(self->masterGain, masterGainDb, dbGain(masterGainDb),
			static_cast<int>(gainSampleRate * 0.01f));
	if (!sidechainEnabled)
		memset(&self->sidechain, 0, sizeof(self->sidechain));
	if (!anyFilterEnabled)
		resetMasterFilterAudioState(self->masterFilter);
	const float configuredSampleRate = NT_globals.sampleRate;
	const float sampleRate = configuredSampleRate > 0.0f ? configuredSampleRate : 48000.0f;
	const float filterQ = filterQFromPercent(self->v[kParamMasterFilterQ]);
	const SvfCoefficients lpCoeffs = lpEnabled
		? makeFilterCoefficients(kFilterLowpass,
			hpCutoffPercentToHz(lpCutoff, sampleRate),
			filterQ)
		: SvfCoefficients();
	const SvfCoefficients hpCoeffs = hpEnabled
		? makeFilterCoefficients(kFilterHighpass,
			hpCutoffPercentToHz(hpCutoff, sampleRate),
			filterQ)
		: SvfCoefficients();
	const float* sidechainKey = inputBus(busFrames, self->v[kParamSidechainKeyInput], numFrames);
	const int effectiveBypass = followBypassOffset(self);
	const int fadeDelaySamples = millisecondsToSamples(5.0f, sampleRate);
	setDelay(self->mainDelay, sidechainEnabled
		? millisecondsToSamples(self->v[kParamSidechainLookahead] * 0.1f, sampleRate) : 0, fadeDelaySamples);
	setDelay(self->bypassDelay, millisecondsToSamples(effectiveBypass * 0.1f, sampleRate), fadeDelaySamples);
	// Cache control coefficients instead of recomputing them every audio block.
	if (self->cachedSampleRate != sampleRate
		|| self->cachedLengthControl != self->v[kParamSidechainEnvLength]
		|| self->cachedCurveControl != self->v[kParamSidechainCurve])
	{
		self->cachedSampleRate = sampleRate;
		self->cachedLengthControl = self->v[kParamSidechainEnvLength];
		self->cachedCurveControl = self->v[kParamSidechainCurve];
		self->envSamples = millisecondsToSamples(envLengthFromNormalized(
			self->cachedLengthControl * 0.001f), sampleRate);
		self->curveBeta = curveToBeta(self->cachedCurveControl * 0.01f);
	}
	const int envSamples = self->envSamples;
	const float beta = self->curveBeta;
	const int smoothSamples = millisecondsToSamples(self->v[kParamSidechainSmooth] * 2.0f, sampleRate);
	const float depth = self->v[kParamSidechainDepth] * 0.01f;
	const OutputPair masterSend = makeOutputPair(busFrames, numFrames,
		self->v[kParamMasterSendL], self->v[kParamMasterSendR]);
	const float* masterReturnL = inputBus(busFrames, self->v[kParamMasterReturnL], numFrames);
	const float* masterReturnR = inputBus(busFrames, self->v[kParamMasterReturnR], numFrames);

	const int fxReturnL[2] = { kParamFx1ReturnL, kParamFx2ReturnL };
	const int fxReturnR[2] = { kParamFx1ReturnR, kParamFx2ReturnR };
	const int fxReturnWidth[2] = { kParamFx1ReturnWidth, kParamFx2ReturnWidth };
	const int fxReturnPath[2] = { kParamFx1ReturnPath, kParamFx2ReturnPath };
	const float* fxLeft[2];
	const float* fxRight[2];
	bool fxStereo[2];
	int fxOutput[2];
	for (int fx = 0; fx < 2; ++fx)
	{
		fxLeft[fx] = inputBus(busFrames, self->v[fxReturnL[fx]], numFrames);
		fxRight[fx] = self->v[fxReturnWidth[fx]] == kWidthStereo
			? inputBus(busFrames, self->v[fxReturnR[fx]], numFrames) : NULL;
		fxStereo[fx] = fxRight[fx] != NULL;
		fxOutput[fx] = self->v[fxReturnPath[fx]] == kOutputPathBypass ? 1 : 0;
	}

	ChannelBlockState channelState[kMaxChannels];
	for (int channel = 0; channel < self->numChannels; ++channel)
	{
		const int base = channelBase(channel);
		ChannelBlockState& state = channelState[channel];
		state.base = base;
		state.left = inputBus(busFrames, self->v[base + kChannelInputL], numFrames);
		state.right = inputBus(busFrames, self->v[base + kChannelInputR], numFrames);
		state.enabled = self->v[base + kChannelEnable] != 0 && state.left != NULL;
		state.stereo = state.right != NULL;
		state.outputIndex = self->v[base + kChannelOutputPath] == kOutputPathBypass ? 1 : 0;

		const int insert1 = insertParameterToState(self->v[base + kChannelInsert1]);
		const int insert2 = insertParameterToState(self->v[base + kChannelInsert2]);
		const int gainDb = self->v[base + kChannelGain];
		const int fx1Percent = self->v[base + kChannelFx1Mix];
		const int fx2Percent = self->v[base + kChannelFx2Mix];

		ChannelRuntime& rt = self->runtime[channel];
		if (!rt.initialised)
			initialiseChannel(rt, insert1, insert2, gainDb, fx1Percent, fx2Percent);
		else
		{
			if (rt.insertState[0] != insert1)
				beginInsertFade(rt, 0, insert1, fadeSamples);
			if (rt.insertState[1] != insert2)
				beginInsertFade(rt, 1, insert2, fadeSamples);
			if (rt.gain.parameterValue != gainDb)
				beginSmooth(rt.gain, gainDb, dbGain(gainDb), fadeSamples);
			if (rt.fx1Mix.parameterValue != fx1Percent)
				beginSmooth(rt.fx1Mix, fx1Percent, percentGain(fx1Percent), fadeSamples);
			if (rt.fx2Mix.parameterValue != fx2Percent)
				beginSmooth(rt.fx2Mix, fx2Percent, percentGain(fx2Percent), fadeSamples);
		}
		state.moving = rt.insertSamplesRemaining[0] > 0
			|| rt.insertSamplesRemaining[1] > 0
			|| rt.gain.samplesRemaining > 0
			|| rt.fx1Mix.samplesRemaining > 0
			|| rt.fx2Mix.samplesRemaining > 0;
		state.fxGains[0] = shapedCrossfade(rt.fx1Mix.value);
		state.fxGains[1] = shapedCrossfade(rt.fx2Mix.value);
		for (int insert = 0; insert < kNumInserts; ++insert)
		{
			for (int routeState = 0; routeState < kNumInsertStates; ++routeState)
				state.routes[insert][routeState] = static_cast<int8_t>(
					selectedRoute(self, channel, insert, routeState));
		}
	}

	for (int frame = 0; frame < numFrames; ++frame)
	{
		float mainLeft = 0.0f;
		float mainRight = 0.0f;
		float bypassLeft = 0.0f;
		float bypassRight = 0.0f;
		// Collect both branches even at zero delay to keep ring history warm.
		outputs[0] = { &mainLeft, &mainRight, true };
		outputs[1] = { &bypassLeft, &bypassRight, true };

		for (int fx = 0; fx < 2; ++fx)
		{
			if (!fxLeft[fx])
				continue;
			const float left = fxLeft[fx][frame];
			const float right = fxRight[fx] ? fxRight[fx][frame] : left;
			addSignal(outputs[fxOutput[fx]], frame, left, right, fxStereo[fx], 1.0f);
		}

		for (int channel = 0; channel < self->numChannels; ++channel)
		{
			ChannelBlockState& state = channelState[channel];
			ChannelRuntime& rt = self->runtime[channel];
			if (state.moving)
			{
				const bool fx1Moving = rt.fx1Mix.samplesRemaining > 0;
				const bool fx2Moving = rt.fx2Mix.samplesRemaining > 0;
				advanceChannel(rt);
				if (fx1Moving)
					state.fxGains[0] = shapedCrossfade(rt.fx1Mix.value);
				if (fx2Moving)
					state.fxGains[1] = shapedCrossfade(rt.fx2Mix.value);
				state.moving = rt.insertSamplesRemaining[0] > 0
					|| rt.insertSamplesRemaining[1] > 0
					|| rt.gain.samplesRemaining > 0
					|| rt.fx1Mix.samplesRemaining > 0
					|| rt.fx2Mix.samplesRemaining > 0;
			}
			if (!state.enabled)
				continue;

			const float left = state.left[frame];
			const float right = state.right ? state.right[frame] : left;
			const CrossfadeGains& fx1 = state.fxGains[0];
			const CrossfadeGains& fx2 = state.fxGains[1];

			if (rt.insertSamplesRemaining[0] == 0 && rt.insertSamplesRemaining[1] == 0)
			{
				processPath(outputs, returnLeft, returnRight, returnStereo,
					frame, left, right, state.stereo,
					state.routes[0][rt.insertState[0]],
					state.routes[1][rt.insertState[1]],
					repeatProtection, state.outputIndex, 1.0f, rt.gain.value,
					fx1, fx2);
			}
			else
			{
				for (int insert1 = 0; insert1 < kNumInsertStates; ++insert1)
				{
					if (rt.insertGain[0][insert1] <= 0.0f)
						continue;
					for (int insert2 = 0; insert2 < kNumInsertStates; ++insert2)
					{
						const float gain = rt.insertGain[0][insert1] * rt.insertGain[1][insert2];
						processPath(outputs, returnLeft, returnRight, returnStereo,
							frame, left, right, state.stereo,
							state.routes[0][insert1],
							state.routes[1][insert2],
							repeatProtection, state.outputIndex, gain,
							rt.gain.value, fx1, fx2);
					}
				}
			}
		}

		advanceSmooth(self->masterGain);
		const float finalGain = self->masterGain.value;

		const float keyMagnitude = sidechainKey ? sidechainKey[frame] : 0.0f;
		const float sidechainGain = sidechainEnabled
			? processSidechain(self->sidechain, keyMagnitude, envSamples, beta, smoothSamples, depth) : 1.0f;
		processDelay(self->mainDelay, mainLeft, mainRight);
		processDelay(self->bypassDelay, bypassLeft, bypassRight);
		float processedMainLeft = mainLeft * sidechainGain;
		float processedMainRight = mainRight * sidechainGain;

		if (masterMode == kMasterSplit)
		{
			if (anyFilterEnabled)
				processMasterFilter(self->masterFilter,
					processedMainLeft, processedMainRight,
					lpEnabled, hpEnabled, lpCoeffs, hpCoeffs);
			addSignal(bypassOutput, frame, bypassLeft, bypassRight, true, finalGain);
			addSignal(mainOutput, frame, processedMainLeft, processedMainRight, true, finalGain);
			continue;
		}

		float masterLeft = processedMainLeft + bypassLeft;
		float masterRight = processedMainRight + bypassRight;
		if (anyFilterEnabled)
			processMasterFilter(self->masterFilter,
				masterLeft, masterRight,
				lpEnabled, hpEnabled, lpCoeffs, hpCoeffs);
		if (masterMode == kMasterInsert)
		{
			addSignal(masterSend, frame, masterLeft, masterRight, true, 1.0f);
			const float returnLeft = masterReturnL ? masterReturnL[frame] : 0.0f;
			const float returnRight = masterReturnR ? masterReturnR[frame] : returnLeft;
			addSignal(mainOutput, frame, returnLeft, returnRight,
				masterReturnR != NULL, finalGain);
		}
		else
		{
			addSignal(mainOutput, frame, masterLeft, masterRight, true, finalGain);
		}
	}
}

_NT_DRAM_SECTION
void serialise(_NT_algorithm* algorithm, _NT_jsonStream& stream)
{
	WitchboardAlgorithm* self = static_cast<WitchboardAlgorithm*>(algorithm);
	// The NT host stores public values/mappings. Trim metadata makes clamps reversible.
	const int autoValue = self->v[kParamSidechainMode] ? self->v[kParamSidechainLookahead] : 0;
	const int trim = self->latencyInitialised && self->v[kParamBypassOffset] == self->previousEffective
		? self->manualTrim : self->v[kParamBypassOffset] - autoValue;
	stream.addMemberName("witchboardLatencyTrim");
	stream.addNumber(trim);

	stream.addMemberName("witchboardNames");
	stream.openObject();
		stream.addMemberName("channels");
		stream.openArray();
		for (int channel = 0; channel < self->numChannels; ++channel)
			stream.addString(self->channelNames[channel]);
		stream.closeArray();
		stream.addMemberName("routes");
		stream.openArray();
		for (int route = 0; route < kNumRoutes; ++route)
			stream.addString(self->routeNames[route]);
		stream.closeArray();
		stream.addMemberName("fx");
		stream.openArray();
		for (int fx = 0; fx < 2; ++fx)
			stream.addString(self->fxNames[fx]);
		stream.closeArray();
		stream.addMemberName("slots");
		stream.openArray();
		for (int insert = 0; insert < kNumInserts; ++insert)
		{
			stream.openArray();
			for (int state = 0; state < kNumInsertStates; ++state)
			{
				if (isAutoSlotName(self->slotNames[insert][state], insert, state))
					stream.addString("");
				else
					stream.addString(self->slotNames[insert][state]);
			}
			stream.closeArray();
		}
		stream.closeArray();
	stream.closeObject();
}

bool parseNames(_NT_jsonParse& parse, char names[][kHardwareNameLength],
	int capacity)
{
	int count = 0;
	if (!parse.numberOfArrayElements(count))
		return false;
	for (int i = 0; i < count; ++i)
	{
		const char* name = NULL;
		if (!parse.string(name))
			return false;
		if (i < capacity)
			copyText(names[i], kHardwareNameLength, name);
	}
	return true;
}

bool parseSlotNames(_NT_jsonParse& parse,
	char names[][kNumInsertStates][kSlotNameLength], int capacity)
{
	int count = 0;
	if (!parse.numberOfArrayElements(count))
		return false;
	for (int insert = 0; insert < count; ++insert)
	{
		int stateCount = 0;
		if (!parse.numberOfArrayElements(stateCount))
			return false;
		for (int state = 0; state < stateCount; ++state)
		{
			const char* name = NULL;
			if (!parse.string(name))
				return false;
			if (insert < capacity && state < kNumInsertStates)
				copyText(names[insert][state], kSlotNameLength, name);
		}
	}
	return true;
}

_NT_DRAM_SECTION
bool deserialise(_NT_algorithm* algorithm, _NT_jsonParse& parse)
{
	WitchboardAlgorithm* self = static_cast<WitchboardAlgorithm*>(algorithm);
	self->latencyInitialised = false;
	self->savedTrim = false;
	int members = 0;
	if (!parse.numberOfObjectMembers(members))
		return false;
	for (int member = 0; member < members; ++member)
	{
		if (parse.matchName("witchboardLatencyTrim"))
		{
			if (!parse.number(self->manualTrim)) return false;
			self->manualTrim = clampInt(self->manualTrim, -100, 1000);
			self->savedTrim = true;
			continue;
		}
		if (!parse.matchName("witchboardNames"))
		{
			if (!parse.skipMember())
				return false;
			continue;
		}

		int nameMembers = 0;
		if (!parse.numberOfObjectMembers(nameMembers))
			return false;
		for (int nameMember = 0; nameMember < nameMembers; ++nameMember)
		{
			if (parse.matchName("channels"))
			{
				if (!parseNames(parse, self->channelNames, self->numChannels))
					return false;
			}
			else if (parse.matchName("routes"))
			{
				if (!parseNames(parse, self->routeNames, kNumRoutes))
					return false;
			}
			else if (parse.matchName("fx"))
			{
				if (!parseNames(parse, self->fxNames, 2))
					return false;
			}
			else if (parse.matchName("slots"))
			{
				if (!parseSlotNames(parse, self->slotNames, kNumInserts))
					return false;
			}
			else if (!parse.skipMember())
				return false;
		}
	}
	return true;
}

bool channelParameterOffset(int parameter, const WitchboardAlgorithm* self,
	int& offset)
{
	if (parameter < kNumGlobalParams
		|| parameter >= kNumGlobalParams + self->numChannels * kNumChannelParams)
		return false;
	offset = (parameter - kNumGlobalParams) % kNumChannelParams;
	return true;
}

int copyParameterString(char* buffer, const char* text)
{
	copyText(buffer, kNT_parameterStringSize, text);
	return strlen(buffer);
}

int channelForParameter(int parameter, const WitchboardAlgorithm* self)
{
	if (parameter < kNumGlobalParams
		|| parameter >= kNumGlobalParams + self->numChannels * kNumChannelParams)
		return -1;
	return (parameter - kNumGlobalParams) / kNumChannelParams;
}

int insertSlotAssignmentParameter(int insert, int state)
{
	if (state <= kInsertDry || state >= kNumInsertStates)
		return -1;
	return (insert == 0 ? kChannelInsert1Slot1 : kChannelInsert2Slot1) + state - 1;
}

const char* insertStateLabel(const WitchboardAlgorithm* self, int parameter,
	int insert, int state)
{
	if (state == kInsertDry)
		return defaultSlotNames[insert][kInsertDry];
	if (!isAutoSlotName(self->slotNames[insert][state], insert, state))
		return self->slotNames[insert][state];
	const int channel = channelForParameter(parameter, self);
	const int assignmentOffset = insertSlotAssignmentParameter(insert, state);
	if (channel >= 0 && assignmentOffset >= 0 && self->v)
	{
		const int route = clampInt(self->v[channelBase(channel) + assignmentOffset],
			0, kNumRoutes - 1);
		return self->routeNames[route];
	}
	return defaultSlotNames[insert][state];
}

_NT_DRAM_SECTION
int parameterString(_NT_algorithm* algorithm, int parameter, int value, char* buffer)
{
	if (parameter == kParamSidechainEnvLength)
		return numberString(buffer, static_cast<unsigned>(envLengthFromNormalized(value * 0.001f) + 0.5f), 0, " ms");

	WitchboardAlgorithm* self = static_cast<WitchboardAlgorithm*>(algorithm);
	int offset = 0;
	if (!channelParameterOffset(parameter, self, offset))
		return 0;

	if (offset == kChannelInsert1 || offset == kChannelInsert2)
	{
		const int insert = offset == kChannelInsert1 ? 0 : 1;
		const int state = insertParameterToState(value);
		return copyParameterString(buffer,
			insertStateLabel(self, parameter, insert, state));
	}

	if ((offset >= kChannelInsert1Slot1 && offset <= kChannelInsert1Slot3)
		|| (offset >= kChannelInsert2Slot1 && offset <= kChannelInsert2Slot3))
	{
		const int route = clampInt(value, 0, kNumRoutes - 1);
		return copyParameterString(buffer, self->routeNames[route]);
	}

	return 0;
}

_NT_DRAM_SECTION
int parameterUiPrefix(_NT_algorithm* algorithm, int parameter, char* buffer)
{
	WitchboardAlgorithm* self = static_cast<WitchboardAlgorithm*>(algorithm);
	if (parameter < kNumGlobalParams
		|| parameter >= kNumGlobalParams + self->numChannels * kNumChannelParams)
		return 0;

	const int channel = (parameter - kNumGlobalParams) / kNumChannelParams + 1;
	const int channelIndex = channel - 1;
	if (!isDefaultChannelName(self, channelIndex))
	{
		int length = 0;
		const char* name = self->channelNames[channelIndex];
		for (; name[length] && length < kNT_parameterUiPrefixSize - 2; ++length)
			buffer[length] = name[length];
		buffer[length++] = ':';
		buffer[length] = 0;
		return length;
	}
	int length = 0;
	if (channel >= 10)
		buffer[length++] = '1';
	buffer[length++] = static_cast<char>('0' + channel % 10);
	buffer[length++] = ':';
	buffer[length] = 0;
	return length;
}

static const _NT_factory witchboardFactory = {
	.guid = NT_MULTICHAR('W', 't', 'b', 'X'),
	.name = "WitchboardX",
	.description = "Serial routing matrix with trigger ducking, latency alignment and master SVF filter",
	.numSpecifications = ARRAY_SIZE(specifications),
	.specifications = specifications,
	.calculateStaticRequirements = NULL,
	.initialise = NULL,
	.calculateRequirements = calculateRequirements,
	.construct = constructWitchboard,
	.parameterChanged = parameterChanged,
	.step = step,
	.draw = NULL,
	.midiRealtime = NULL,
	.midiMessage = NULL,
	.tags = kNT_tagUtility,
	.hasCustomUi = NULL,
	.customUi = NULL,
	.setupUi = NULL,
	.serialise = serialise,
	.deserialise = deserialise,
	.midiSysEx = NULL,
	.parameterUiPrefix = parameterUiPrefix,
	.parameterString = parameterString,
};

} // namespace

extern "C" _NT_DRAM_SECTION uintptr_t pluginEntry(_NT_selector selector, uint32_t data)
{
	switch (selector)
	{
	case kNT_selector_version:
		return kNT_apiVersionCurrent;
	case kNT_selector_numFactories:
		return 1;
	case kNT_selector_factoryInfo:
		return data == 0 ? reinterpret_cast<uintptr_t>(&witchboardFactory) : 0;
	}
	return 0;
}
