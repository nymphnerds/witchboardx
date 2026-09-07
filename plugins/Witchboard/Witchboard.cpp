#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <new>

#include <distingnt/api.h>
#include <distingnt/serialisation.h>

namespace
{

constexpr int kMaxChannels = 11;
constexpr int kNumRoutes = 5;
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

enum KeyMode
{
	kKeyTrigger,
	kKeyGate,
	kKeyAudio,
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
constexpr int kParamSidechainKeyMode = kParamSidechainMode + 2;
constexpr int kParamSidechainDepth = kParamSidechainMode + 3;
constexpr int kParamSidechainAttack = kParamSidechainMode + 4;
constexpr int kParamSidechainRelease = kParamSidechainMode + 5;
constexpr int kParamSidechainReleaseCurve = kParamSidechainMode + 6;
constexpr int kParamSidechainMakeup = kParamSidechainMode + 7;
constexpr int kParamMasterFilterEnable = kParamSidechainMakeup + 1;
constexpr int kParamMasterFilterHpCutoff = kParamMasterFilterEnable + 1;
constexpr int kParamMasterFilterLpCutoff = kParamMasterFilterEnable + 2;
constexpr int kParamMasterFilterQ = kParamMasterFilterEnable + 3;
constexpr int kParamMasterFilterSweep = kParamMasterFilterEnable + 4;
constexpr int kParamMasterMode = kParamMasterFilterSweep + 1;
constexpr int kParamMasterSendL = kParamMasterMode + 1;
constexpr int kParamMasterSendR = kParamMasterMode + 2;
constexpr int kParamMasterReturnL = kParamMasterMode + 3;
constexpr int kParamMasterReturnR = kParamMasterMode + 4;
constexpr int kParamEq1Freq = kParamMasterReturnR + 1;
constexpr int kParamEq1Gain = kParamEq1Freq + 1;
constexpr int kParamEq1Q = kParamEq1Freq + 2;
constexpr int kParamEq2Freq = kParamEq1Freq + 3;
constexpr int kParamEq2Gain = kParamEq1Freq + 4;
constexpr int kParamEq2Q = kParamEq1Freq + 5;
constexpr int kParamEq3Freq = kParamEq1Freq + 6;
constexpr int kParamEq3Gain = kParamEq1Freq + 7;
constexpr int kParamEq3Q = kParamEq1Freq + 8;
constexpr int kNumEqParams = 9;
constexpr int kNumGlobalParams = kParamEq1Freq + kNumEqParams;

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
constexpr int kFinalOutputParams = kParamFx1L - kParamMainL;
constexpr int kFxSetupParams = kParamSidechainMode - kParamFx1L;
constexpr int kMasterPageParams = kNumGlobalParams - kParamSidechainMode;
constexpr int kMaxParams = kNumGlobalParams + kMaxChannels * kNumChannelParams;
static_assert(kMaxParams == 242, "11-channel parameter budget changed");
static_assert(kMaxParams < 256, "parameter indices exceed NT limit");
static_assert(kNumGlobalParams == 77, "global parameter count changed");
static_assert(kParamMainL == 31, "final output page indices changed");
static_assert(kParamFx1L == 35, "FX setup page indices changed");
static_assert(kParamRepeatProtection == 49, "repeat protection index changed");
static_assert(kParamSidechainMode == 50, "master page indices changed");
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

static char const* const keyModeStrings[] = {
	"Trigger", "Gate", "Audio",
};

static char const* const widthStrings[] = {
	"Mono", "Stereo",
};

static char const* const channelPageNames[kMaxChannels] = {
	"Channel 1", "Channel 2", "Channel 3", "Channel 4",
	"Channel 5", "Channel 6", "Channel 7", "Channel 8",
	"Channel 9", "Channel 10", "Channel 11",
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
	"Route A", "Route B", "Route C", "Route D", "Route E",
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
};

static const uint8_t finalOutputPageParams[kFinalOutputParams] = {
	kParamMainL, kParamMainR,
	kParamBypassL, kParamBypassR,
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
	kParamSidechainMode, kParamSidechainKeyInput,
	kParamSidechainKeyMode, kParamSidechainDepth,
	kParamSidechainAttack, kParamSidechainRelease,
	kParamSidechainReleaseCurve, kParamSidechainMakeup,
	kParamMasterFilterEnable, kParamMasterFilterHpCutoff,
	kParamMasterFilterLpCutoff, kParamMasterFilterQ,
	kParamMasterFilterSweep,
	kParamEq1Freq, kParamEq1Gain, kParamEq1Q,
	kParamEq2Freq, kParamEq2Gain, kParamEq2Q,
	kParamEq3Freq, kParamEq3Gain, kParamEq3Q,
	kParamMasterMode,
	kParamMasterSendL, kParamMasterSendR,
	kParamMasterReturnL, kParamMasterReturnR,
};

static const _NT_specification specifications[] = {
	{ .name = "Channels", .min = 1, .max = kMaxChannels, .def = 4, .type = kNT_typeGeneric },
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

struct SidechainRuntime
{
	bool keyHigh;
	int stage;
	int samplesRemaining;
	int samplesTotal;
	float startGain;
	float targetGain;
	float gain;
	float detector;
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

// Double precision prevents cancellation at low frequencies/high sample rates.
struct EqCoefficients { double b0, b1, b2, a1, a2; };
struct EqState { double x1, x2, y1, y2; };
struct EqBandRuntime
{
	float controls[3];
	float targets[3];
	EqCoefficients coefficients;
	EqState left, right;
	bool dirty;
};
struct MasterEqRuntime
{
	EqBandRuntime bands[3];
	float sampleRate;
	float smoothing;
	int untilUpdate;
	bool initialised;
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
	MasterEqRuntime masterEq;
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
	memset(&masterEq, 0, sizeof(masterEq));
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
	for (int route = 0; route < kNumRoutes; ++route)
		copyText(routeNames[route], kHardwareNameLength, defaultRouteNames[route]);
	for (int fx = 0; fx < 2; ++fx)
		copyText(fxNames[fx], kHardwareNameLength, defaultFxNames[fx]);
	for (int insert = 0; insert < kNumInserts; ++insert)
		for (int state = 0; state < kNumInsertStates; ++state)
			copyText(slotNames[insert][state], kSlotNameLength,
				defaultSlotNames[insert][state]);
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
	setInput(parameterDefs[kParamSidechainKeyInput], "SC key input");
	setParameter(parameterDefs[kParamSidechainKeyMode], "SC key mode", 0, 2,
		kKeyTrigger, kNT_unitEnum, keyModeStrings);
	setParameter(parameterDefs[kParamSidechainDepth], "SC depth", 0, 100, 90,
		kNT_unitPercent);
	setParameter(parameterDefs[kParamSidechainAttack], "SC attack", 0, 50, 0,
		kNT_unitMs);
	setParameter(parameterDefs[kParamSidechainRelease], "SC release", 5, 800, 120,
		kNT_unitMs);
	setParameter(parameterDefs[kParamSidechainReleaseCurve], "SC curve", -300, 200, -120,
		kNT_unitNone);
	setParameter(parameterDefs[kParamSidechainMakeup], "SC makeup", 0, 6, 2,
		kNT_unitDb);

	setParameter(parameterDefs[kParamMasterFilterEnable], "Filter enable", 0, 1, 0,
		kNT_unitEnum, offOnStrings);
	setParameter(parameterDefs[kParamMasterFilterHpCutoff], "HP limit", 0, 100, 100,
		kNT_unitPercent);
	setParameter(parameterDefs[kParamMasterFilterLpCutoff], "LP limit", 0, 100, 0,
		kNT_unitPercent);
	setParameter(parameterDefs[kParamMasterFilterQ], "Filter Q", 0, 100, 0,
		kNT_unitPercent);
	setParameter(parameterDefs[kParamMasterFilterSweep], "Filter sweep", -100, 100, 0,
		kNT_unitPercent);

	setParameter(parameterDefs[kParamMasterMode], "Master output", 0, 2, 0,
		kNT_unitEnum, masterModeStrings);
	setOutput(parameterDefs[kParamMasterSendL], "Master send L");
	setOutput(parameterDefs[kParamMasterSendR], "Master send R");
	setInput(parameterDefs[kParamMasterReturnL], "Master return L");
	setInput(parameterDefs[kParamMasterReturnR], "Master return R");

	// Log controls map nominally to 20 Hz..20 kHz and Q 0.25..12.
	static const char* const eqNames[kNumEqParams] = {
		"EQ1 Freq", "EQ1 Gain", "EQ1 Q", "EQ2 Freq", "EQ2 Gain", "EQ2 Q",
		"EQ3 Freq", "EQ3 Gain", "EQ3 Q",
	};
	const int eqDefaults[kNumEqParams] = { 259, 0, 358, 566, 0, 358, 867, 0, 358 };
	for (int i = 0; i < kNumEqParams; ++i)
		setParameter(parameterDefs[kParamEq1Freq + i], eqNames[i],
			i % 3 == 1 ? -18 : 0, i % 3 == 1 ? 18 : 1000, eqDefaults[i],
			i % 3 == 1 ? kNT_unitDb : kNT_unitHasStrings);

	for (int channel = 0; channel < kMaxChannels; ++channel)
	{
		const int base = channelBase(channel);
		setParameter(parameterDefs[base + kChannelEnable],
			channelSuffixes[kChannelEnable], 0, 1, 1, kNT_unitEnum, offOnStrings);
		setInput(parameterDefs[base + kChannelInputL], channelSuffixes[kChannelInputL]);
		setInput(parameterDefs[base + kChannelInputR], channelSuffixes[kChannelInputR]);
		setParameter(parameterDefs[base + kChannelGain],
			channelSuffixes[kChannelGain], -60, 0, 0, kNT_unitDb_minInf);
		setParameter(parameterDefs[base + kChannelInsert1],
			channelSuffixes[kChannelInsert1], 0, kInsertParameterMax, 0,
			kNT_unitEnum, insertStateStrings);
		for (int slot = 0; slot < 3; ++slot)
			setParameter(parameterDefs[base + kChannelInsert1Slot1 + slot],
				channelSuffixes[kChannelInsert1Slot1 + slot], 0, kNumRoutes - 1,
				slot, kNT_unitHasStrings);
		setParameter(parameterDefs[base + kChannelFx1Mix],
			channelSuffixes[kChannelFx1Mix], 0, 100, 0, kNT_unitPercent);
		setParameter(parameterDefs[base + kChannelInsert2],
			channelSuffixes[kChannelInsert2], 0, kInsertParameterMax, 0,
			kNT_unitEnum, insertStateStrings);
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
			.name = channelPageNames[channel],
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

float shapeReleaseProgress(float progress, int curveParam)
{
	progress = clampFloat(progress, 0.0f, 1.0f);
	const float amount = clampFloat(fabsf(curveParam) / 300.0f, 0.0f, 1.0f);
	if (amount <= 0.0f)
		return progress;

	const float x = curveParam < 0 ? progress : 1.0f - progress;
	const float x2 = x * x;
	const float x4 = x2 * x2;
	const float x8 = x4 * x4;
	const float steep = x8 * x;
	const float shaped = x + (steep - x) * amount;
	if (curveParam < 0)
		return shaped;
	return 1.0f - shaped;
}

int timeMsToSamples(int milliseconds, int sampleRate, int minimumSamples)
{
	const int samples = milliseconds * sampleRate / 1000;
	return samples < minimumSamples ? minimumSamples : samples;
}

enum SidechainStage
{
	kSidechainIdle,
	kSidechainAttack,
	kSidechainRelease,
};

constexpr float kSidechainTriggerThreshold = 0.1f;
constexpr float kPi = 3.14159265358979323846f;

void startSidechainStage(SidechainRuntime& sidechain, int stage, float targetGain,
	int totalSamples)
{
	sidechain.stage = stage;
	sidechain.startGain = sidechain.gain;
	sidechain.targetGain = clampFloat(targetGain, 0.0f, 1.0f);
	sidechain.samplesTotal = totalSamples < 1 ? 1 : totalSamples;
	sidechain.samplesRemaining = sidechain.samplesTotal;
}

void startSidechainEnvelope(SidechainRuntime& sidechain, int depthPercent,
	int attackSamples)
{
	const float targetGain = 1.0f - clampFloat(depthPercent * 0.01f, 0.0f, 1.0f);
	if (attackSamples <= 1)
	{
		sidechain.gain = targetGain;
		sidechain.stage = kSidechainRelease;
		sidechain.samplesRemaining = 0;
		sidechain.samplesTotal = 1;
		sidechain.startGain = targetGain;
		sidechain.targetGain = 1.0f;
		return;
	}

	startSidechainStage(sidechain, kSidechainAttack, targetGain, attackSamples);
}

void advanceSidechainEnvelope(SidechainRuntime& sidechain, int releaseSamples,
	int curveParam)
{
	if (sidechain.gain <= 0.0f && sidechain.stage == kSidechainIdle)
		sidechain.gain = 1.0f;

	if (sidechain.stage == kSidechainAttack)
	{
		if (sidechain.samplesRemaining <= 0)
		{
			sidechain.gain = sidechain.targetGain;
			startSidechainStage(sidechain, kSidechainRelease, 1.0f, releaseSamples);
		}
		else
		{
			const float progress = 1.0f
				- static_cast<float>(sidechain.samplesRemaining) / sidechain.samplesTotal;
			sidechain.gain = sidechain.startGain
				+ (sidechain.targetGain - sidechain.startGain)
					* clampFloat(progress, 0.0f, 1.0f);
			--sidechain.samplesRemaining;
		}
		return;
	}

	if (sidechain.stage == kSidechainRelease)
	{
		if (sidechain.samplesRemaining <= 0)
		{
			if (sidechain.targetGain != 1.0f)
				startSidechainStage(sidechain, kSidechainRelease, 1.0f, releaseSamples);
			else
			{
				sidechain.gain = 1.0f;
				sidechain.stage = kSidechainIdle;
			}
			return;
		}

		const float progress = 1.0f
			- static_cast<float>(sidechain.samplesRemaining) / sidechain.samplesTotal;
		const float shapedProgress = shapeReleaseProgress(progress, curveParam);
		sidechain.gain = sidechain.startGain
			+ (sidechain.targetGain - sidechain.startGain) * shapedProgress;
		--sidechain.samplesRemaining;
	}
}

float processSidechain(SidechainRuntime& sidechain, float keyMagnitude, int keyMode,
	int depthPercent, int attackMs, int releaseMs, int curveParam)
{
	if (sidechain.gain <= 0.0f && sidechain.stage == kSidechainIdle)
		sidechain.gain = 1.0f;

	const float absoluteKey = fabsf(keyMagnitude);
	const bool keyHigh = absoluteKey > kSidechainTriggerThreshold;
	const int configuredSampleRate = static_cast<int>(NT_globals.sampleRate);
	const int sampleRate = configuredSampleRate > 0 ? configuredSampleRate : 48000;
	const int attackSamples = timeMsToSamples(attackMs, sampleRate, 0);
	const int releaseSamples = timeMsToSamples(releaseMs, sampleRate, 1);
	const float depth = clampFloat(depthPercent * 0.01f, 0.0f, 1.0f);

	if (keyMode == kKeyAudio)
	{
		const float target = clampFloat(absoluteKey, 0.0f, 1.0f);
		if (target >= sidechain.detector)
		{
			if (attackSamples <= 1)
				sidechain.detector = target;
			else
				sidechain.detector += (target - sidechain.detector)
					/ static_cast<float>(attackSamples);
		}
		else
		{
			const float curve = static_cast<float>(curveParam);
			const float releaseScale = curve < 0.0f
				? 1.0f / (1.0f + fabsf(curve) * 0.03f)
				: 1.0f + curve * 0.02f;
			sidechain.detector += (target - sidechain.detector)
				* releaseScale / static_cast<float>(releaseSamples);
		}

		sidechain.keyHigh = keyHigh;
		sidechain.stage = kSidechainIdle;
		sidechain.gain = 1.0f - depth * clampFloat(sidechain.detector, 0.0f, 1.0f);
		return clampFloat(sidechain.gain, 0.0f, 1.0f);
	}

	if (keyMode == kKeyTrigger)
	{
		if (keyHigh && !sidechain.keyHigh)
		{
			startSidechainEnvelope(sidechain, depthPercent, attackSamples);
			if (sidechain.stage == kSidechainRelease && sidechain.samplesRemaining == 0)
				startSidechainStage(sidechain, kSidechainRelease, 1.0f, releaseSamples);
		}
		else
			advanceSidechainEnvelope(sidechain, releaseSamples, curveParam);
	}
	else
	{
		if (keyHigh && !sidechain.keyHigh)
		{
			startSidechainEnvelope(sidechain, depthPercent, attackSamples);
			if (sidechain.stage == kSidechainRelease && sidechain.samplesRemaining == 0)
				startSidechainStage(sidechain, kSidechainRelease, 1.0f, releaseSamples);
		}
		else if (keyHigh && sidechain.stage == kSidechainAttack)
			advanceSidechainEnvelope(sidechain, releaseSamples, curveParam);
		else if (keyHigh)
		{
			sidechain.gain = 1.0f - depth;
			sidechain.stage = kSidechainIdle;
		}
		else
		{
			if (sidechain.stage == kSidechainIdle && sidechain.gain < 1.0f)
				startSidechainStage(sidechain, kSidechainRelease, 1.0f, releaseSamples);
			advanceSidechainEnvelope(sidechain, releaseSamples, curveParam);
		}
	}

	sidechain.keyHigh = keyHigh;
	return clampFloat(sidechain.gain, 0.0f, 1.0f);
}

/*
Peaking design and oneSided Q compensation adapted from
https://github.com/Signalsmith-Audio/dsp/blob/main/filters.h
Only the oneSided peaking path is ported; no library dependency.

MIT License
Copyright (c) 2021 Geraint Luff / Signalsmith Audio Ltd.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
constexpr double kEqPi = 3.14159265358979323846;
constexpr int kEqControlInterval = 16;

float eqFrequency(float control, float sampleRate)
{
	const float maximum = sampleRate * 0.45f < 20000.0f ? sampleRate * 0.45f : 20000.0f;
	return clampFloat(20.0f * powf(1000.0f, clampFloat(control, 0, 1000) * 0.001f),
		20.0f, maximum);
}

float eqQ(float control)
{
	return 0.25f * powf(48.0f, clampFloat(control, 0, 1000) * 0.001f);
}

// Range-reduced Taylor polynomials on [0, pi/2]. Avoid new NT libm imports.
// The first omitted sine term is < 2e-18 on this interval.
double eqSinHalf(double x)
{
	const double x2 = x*x;
	double polynomial = 1.0/51090942171709440000.0;
	polynomial = -1.0/121645100408832000.0 + x2*polynomial;
	polynomial = 1.0/355687428096000.0 + x2*polynomial;
	polynomial = -1.0/1307674368000.0 + x2*polynomial;
	polynomial = 1.0/6227020800.0 + x2*polynomial;
	polynomial = -1.0/39916800.0 + x2*polynomial;
	polynomial = 1.0/362880.0 + x2*polynomial;
	polynomial = -1.0/5040.0 + x2*polynomial;
	polynomial = 1.0/120.0 + x2*polynomial;
	polynomial = -1.0/6.0 + x2*polynomial;
	return x*(1 + x2*polynomial);
}

double eqTanHalf(double x)
{
	return eqSinHalf(x) / eqSinHalf(kEqPi*0.5 - x);
}

EqCoefficients makeEqCoefficients(float frequency, float gainDb, float q, float sampleRate)
{
	if (gainDb == 0)
		return { 1, 0, 0, 0, 0 };
	const double halfW = kEqPi * clampFloat(frequency, 20, sampleRate*0.45f) / sampleRate;
	const double sinHalf = eqSinHalf(halfW);
	const double cosHalf = eqSinHalf(kEqPi*0.5 - halfW);
	const double sinW = 2*sinHalf*cosHalf;
	const double cosW = cosHalf*cosHalf - sinHalf*sinHalf;
	const double inv2Q = 0.5 / clampFloat(q, 0.25f, 12);
	// Rationalised form of sqrt(1 + inv2Q^2) - inv2Q.
	// Newton square root on [1, 5], five iterations from a bounded estimate.
	// Avoid even a conditional sqrtf import from the compiler's libm fallback.
	const double squared = 1 + inv2Q*inv2Q;
	double root = 1 + inv2Q;
	for (int i = 0; i < 5; ++i) root = 0.5*(root + squared/root);
	const double f1Factor = 1 / (root + inv2Q);
	const double ctRatio = eqTanHalf(halfW*f1Factor) * cosHalf/sinHalf;
	const double compensated = 0.5/ctRatio - 0.5*ctRatio;
	const double alpha = sinW * compensated;
	const double a = powf(10, clampFloat(gainDb, -18, 18)*0.025f);
	const double normalise = 1 / (1 + alpha/a);
	return { (1 + alpha*a)*normalise, -2*cosW*normalise,
		(1 - alpha*a)*normalise, -2*cosW*normalise, (1 - alpha/a)*normalise };
}

double processEqBiquad(const EqCoefficients& c, EqState& state, double x)
{
	const double y = c.b0*x + c.b1*state.x1 + c.b2*state.x2
		- c.a1*state.y1 - c.a2*state.y2;
	state.x2 = state.x1;
	state.x1 = x;
	state.y2 = state.y1;
	state.y1 = y;
	return y;
}

bool prepareMasterEq(MasterEqRuntime& eq, const int16_t* parameters, float sampleRate)
{
	const bool rateChanged = !eq.initialised || eq.sampleRate != sampleRate;
	if (rateChanged)
	{
		eq.sampleRate = sampleRate;
		// 10 ms time constant, independent of host block size.
		eq.smoothing = 1 - powf(2.718281828459045f,
			-kEqControlInterval / (0.01f*sampleRate));
		eq.untilUpdate = 0;
	}
	bool active = false;
	for (int band = 0; band < 3; ++band)
	{
		EqBandRuntime& b = eq.bands[band];
		for (int field = 0; field < 3; ++field)
		{
			b.targets[field] = clampInt(parameters[kParamEq1Freq + band*3 + field],
				field == 1 ? -18 : 0, field == 1 ? 18 : 1000);
			if (!eq.initialised)
				b.controls[field] = b.targets[field];
		}
		b.dirty = b.dirty || rateChanged;
		active = active || b.targets[1] != 0 || b.controls[1] != 0;
	}
	eq.initialised = true;
	return active;
}

void advanceMasterEq(MasterEqRuntime& eq)
{
	if (eq.untilUpdate-- > 0)
		return;
	eq.untilUpdate = kEqControlInterval - 1;
	for (int band = 0; band < 3; ++band)
	{
		EqBandRuntime& b = eq.bands[band];
		bool changed = b.dirty;
		for (int field = 0; field < 3; ++field)
		{
			const float delta = b.targets[field] - b.controls[field];
			if (delta != 0)
			{
				const float next = b.controls[field] + eq.smoothing*delta;
				b.controls[field] = fabsf(delta) < 0.00001f || next == b.controls[field]
					? b.targets[field] : next;
				changed = true;
			}
		}
		if (changed)
		{
			b.coefficients = makeEqCoefficients(eqFrequency(b.controls[0], eq.sampleRate),
				b.controls[1], eqQ(b.controls[2]), eq.sampleRate);
			b.dirty = false;
		}
	}
}

void processMasterEq(MasterEqRuntime& eq, float& left, float& right)
{
	double l = left, r = right;
	for (int band = 0; band < 3; ++band)
	{
		EqBandRuntime& b = eq.bands[band];
		l = processEqBiquad(b.coefficients, b.left, l);
		r = processEqBiquad(b.coefficients, b.right, r);
	}
	left = static_cast<float>(l);
	right = static_cast<float>(r);
}

// Small decimal formatter avoids introducing printf into the NT object.
int eqNumberString(char* buffer, unsigned value, int decimals, const char* suffix)
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

void calculateRequirements(_NT_algorithmRequirements& requirements, const int32_t* specs)
{
	const int channels = clampInt(specs[0], 1, kMaxChannels);
	requirements.numParameters = kNumGlobalParams + channels * kNumChannelParams;
	requirements.sram = static_cast<uint32_t>(requiredSram(channels));
	requirements.dram = static_cast<uint32_t>(requiredDram(channels));
	requirements.dtc = 0;
	requirements.itc = 0;
}

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
	const float eqSampleRate = NT_globals.sampleRate > 0 ? NT_globals.sampleRate : 48000;
	const bool eqActive = prepareMasterEq(self->masterEq, self->v, eqSampleRate);
	const bool busProcessing = sidechainEnabled || anyFilterEnabled || eqActive || masterMode != kMasterSplit;
	if (!sidechainEnabled)
		memset(&self->sidechain, 0, sizeof(self->sidechain));
	if (!anyFilterEnabled)
		resetMasterFilterAudioState(self->masterFilter);
	const float sidechainMakeup = sidechainEnabled
		? dbGain(self->v[kParamSidechainMakeup])
		: 1.0f;
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
	const int sidechainKeyMode = clampInt(self->v[kParamSidechainKeyMode], kKeyTrigger, kKeyAudio);
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
		if (busProcessing)
		{
			outputs[0] = { &mainLeft, &mainRight, true };
			outputs[1] = { &bypassLeft, &bypassRight, true };
		}

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

		advanceMasterEq(self->masterEq);
		if (!busProcessing)
			continue;

		const float keyMagnitude = sidechainKey ? sidechainKey[frame] : 0.0f;
		const float sidechainGain = sidechainEnabled
			? processSidechain(self->sidechain, keyMagnitude, sidechainKeyMode,
				self->v[kParamSidechainDepth], self->v[kParamSidechainAttack],
				self->v[kParamSidechainRelease], self->v[kParamSidechainReleaseCurve])
			: 1.0f;
		float processedMainLeft = mainLeft * sidechainGain * sidechainMakeup;
		float processedMainRight = mainRight * sidechainGain * sidechainMakeup;

		if (masterMode == kMasterSplit)
		{
			if (anyFilterEnabled)
				processMasterFilter(self->masterFilter,
					processedMainLeft, processedMainRight,
					lpEnabled, hpEnabled, lpCoeffs, hpCoeffs);
			if (eqActive)
				processMasterEq(self->masterEq, processedMainLeft, processedMainRight);
			addSignal(bypassOutput, frame, bypassLeft, bypassRight, true, 1.0f);
			addSignal(mainOutput, frame, processedMainLeft, processedMainRight, true, 1.0f);
			continue;
		}

		float masterLeft = processedMainLeft + bypassLeft;
		float masterRight = processedMainRight + bypassRight;
		if (anyFilterEnabled)
			processMasterFilter(self->masterFilter,
				masterLeft, masterRight,
				lpEnabled, hpEnabled, lpCoeffs, hpCoeffs);
		if (eqActive)
			processMasterEq(self->masterEq, masterLeft, masterRight);
		if (masterMode == kMasterInsert)
		{
			addSignal(masterSend, frame, masterLeft, masterRight, true, 1.0f);
			const float returnLeft = masterReturnL ? masterReturnL[frame] : 0.0f;
			const float returnRight = masterReturnR ? masterReturnR[frame] : returnLeft;
			addSignal(mainOutput, frame, returnLeft, returnRight,
				masterReturnR != NULL, 1.0f);
		}
		else
		{
			addSignal(mainOutput, frame, masterLeft, masterRight, true, 1.0f);
		}
	}
}

void serialise(_NT_algorithm* algorithm, _NT_jsonStream& stream)
{
	WitchboardAlgorithm* self = static_cast<WitchboardAlgorithm*>(algorithm);
	stream.addMemberName("witchboardNames");
	stream.openObject();
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
				stream.addString(self->slotNames[insert][state]);
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

bool deserialise(_NT_algorithm* algorithm, _NT_jsonParse& parse)
{
	WitchboardAlgorithm* self = static_cast<WitchboardAlgorithm*>(algorithm);
	int members = 0;
	if (!parse.numberOfObjectMembers(members))
		return false;
	for (int member = 0; member < members; ++member)
	{
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
			if (parse.matchName("routes"))
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

int parameterString(_NT_algorithm* algorithm, int parameter, int value, char* buffer)
{
	if (parameter >= kParamEq1Freq && parameter < kNumGlobalParams)
	{
		const int field = (parameter - kParamEq1Freq) % 3;
		if (field == 0)
		{
			const float rate = NT_globals.sampleRate > 0 ? NT_globals.sampleRate : 48000;
			const float hz = eqFrequency(value, rate);
			return hz < 1000 ? eqNumberString(buffer, static_cast<unsigned>(hz + 0.5f), 0, " Hz")
				: eqNumberString(buffer, static_cast<unsigned>(hz*0.01f + 0.5f), 2, " kHz");
		}
		if (field == 2)
			return eqNumberString(buffer, static_cast<unsigned>(eqQ(value)*100 + 0.5f), 2, "");
		return 0;
	}

	WitchboardAlgorithm* self = static_cast<WitchboardAlgorithm*>(algorithm);
	int offset = 0;
	if (!channelParameterOffset(parameter, self, offset))
		return 0;

	if (offset == kChannelInsert1 || offset == kChannelInsert2)
	{
		const int insert = offset == kChannelInsert1 ? 0 : 1;
		const int state = insertParameterToState(value);
		return copyParameterString(buffer, self->slotNames[insert][state]);
	}

	if ((offset >= kChannelInsert1Slot1 && offset <= kChannelInsert1Slot3)
		|| (offset >= kChannelInsert2Slot1 && offset <= kChannelInsert2Slot3))
	{
		const int route = clampInt(value, 0, kNumRoutes - 1);
		return copyParameterString(buffer, self->routeNames[route]);
	}

	return 0;
}

int parameterUiPrefix(_NT_algorithm* algorithm, int parameter, char* buffer)
{
	WitchboardAlgorithm* self = static_cast<WitchboardAlgorithm*>(algorithm);
	if (parameter < kNumGlobalParams
		|| parameter >= kNumGlobalParams + self->numChannels * kNumChannelParams)
		return 0;

	const int channel = (parameter - kNumGlobalParams) / kNumChannelParams + 1;
	int length = 0;
	if (channel >= 10)
		buffer[length++] = '1';
	buffer[length++] = static_cast<char>('0' + channel % 10);
	buffer[length++] = ':';
	buffer[length] = 0;
	return length;
}

static const _NT_factory witchboardFactory = {
	.guid = NT_MULTICHAR('W', 't', 'E', 'Q'),
	.name = "Witchboard EQ",
	.description = "Serial routing matrix with sidechain gain shaping and master SVF filter / 3-band EQ",
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

extern "C" uintptr_t pluginEntry(_NT_selector selector, uint32_t data)
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
