#include <new>
#include <cstring>
#include <distingnt/api.h>

namespace {

enum ParamIndex {
    kProfile = 0,
    kProfileCount,
    kHome,
    kMidiChannel,
    kMidiCC,
    kNumParams
};

static const int kMaxProfiles = 32;

static const _NT_parameter kParameterDefaults[kNumParams] = {
    { .name = "Profile",       .min = 1, .max = 32, .def = 1,   .unit = kNT_unitHasStrings, .scaling = kNT_scalingNone, .enumStrings = NULL },
    { .name = "Profile count", .min = 1, .max = 32, .def = 2,   .unit = kNT_unitNone,       .scaling = kNT_scalingNone, .enumStrings = NULL },
    { .name = "Home",          .min = 1, .max = 32, .def = 1,   .unit = kNT_unitHasStrings, .scaling = kNT_scalingNone, .enumStrings = NULL },
    { .name = "MIDI channel",  .min = 1, .max = 16, .def = 1,   .unit = kNT_unitNone,       .scaling = kNT_scalingNone, .enumStrings = NULL },
    { .name = "MIDI CC",       .min = 0, .max = 127,.def = 119, .unit = kNT_unitNone,       .scaling = kNT_scalingNone, .enumStrings = NULL },
};

static const uint8_t kProfilePageParams[] = { kProfile, kProfileCount, kHome };
static const uint8_t kMidiPageParams[] = { kMidiChannel, kMidiCC };

static const _NT_parameterPage kPages[] = {
    { .name = "Profile", .numParams = ARRAY_SIZE(kProfilePageParams), .group = 0, .unused = {0,0}, .params = kProfilePageParams },
    { .name = "MIDI",    .numParams = ARRAY_SIZE(kMidiPageParams),    .group = 0, .unused = {0,0}, .params = kMidiPageParams },
};

static const _NT_parameterPages kParameterPages = {
    .numPages = ARRAY_SIZE(kPages),
    .pages = kPages,
};

struct LoopyDial : public _NT_algorithm {
    LoopyDial() : ready(false), changingInternally(false), feedbackProfile(0) {}

    _NT_parameter parameterDefs[kNumParams];
    bool ready;
    bool changingInternally;
    int16_t feedbackProfile;
};

static int clampInt(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static int profileToMidi(int zeroBasedIndex, int count) {
    count = clampInt(count, 1, kMaxProfiles);
    if (count <= 1) return 0;
    zeroBasedIndex = clampInt(zeroBasedIndex, 0, count - 1);
    const int d = count - 1;
    return clampInt((128 * zeroBasedIndex + d / 2) / d, 0, 127);
}

static int midiToProfile(int value, int count) {
    value = clampInt(value, 0, 127);
    count = clampInt(count, 1, kMaxProfiles);
    if (count <= 1) return 1;

    int bestIndex = 0;
    int bestDistance = 1000;
    for (int i = 0; i < count; ++i) {
        int distance = profileToMidi(i, count) - value;
        if (distance < 0) distance = -distance;
        if (distance < bestDistance) {
            bestDistance = distance;
            bestIndex = i;
        }
    }
    return bestIndex + 1;
}

static bool validAlgorithmIndex(_NT_algorithm* self, int& index) {
    index = NT_algorithmIndex(self);
    return index >= 0;
}

static void sendProfile(LoopyDial* self) {
    if (!self->v) return;

    const int count = clampInt(self->v[kProfileCount], 1, kMaxProfiles);
    const int profile = clampInt(self->v[kProfile], 1, count);
    const int channel = clampInt(self->v[kMidiChannel], 1, 16);
    const int cc = clampInt(self->v[kMidiCC], 0, 127);

    NT_sendMidi3ByteMessage(
        kNT_destinationUSB,
        static_cast<uint8_t>(0xB0 | (channel - 1)),
        static_cast<uint8_t>(cc),
        static_cast<uint8_t>(profileToMidi(profile - 1, count)));
}

static void applyScaledRanges(LoopyDial* self, bool resend) {
    if (!self->v) return;

    int algorithmIndex = -1;
    if (!validAlgorithmIndex(self, algorithmIndex)) return;

    const int count = clampInt(self->v[kProfileCount], 1, kMaxProfiles);
    self->changingInternally = true;

    if (self->v[kProfile] > count) {
        NT_setParameterFromAudio(
            static_cast<uint32_t>(algorithmIndex),
            static_cast<uint32_t>(kProfile) + NT_parameterOffset(),
            static_cast<int16_t>(count));
    }

    if (self->v[kHome] > count) {
        NT_setParameterFromAudio(
            static_cast<uint32_t>(algorithmIndex),
            static_cast<uint32_t>(kHome) + NT_parameterOffset(),
            static_cast<int16_t>(count));
    }

    if (self->parameterDefs[kProfile].max != count) {
        self->parameterDefs[kProfile].max = static_cast<int16_t>(count);
        NT_updateParameterDefinition(
            static_cast<uint32_t>(algorithmIndex),
            static_cast<uint32_t>(kProfile));
    }

    if (self->parameterDefs[kHome].max != count) {
        self->parameterDefs[kHome].max = static_cast<int16_t>(count);
        NT_updateParameterDefinition(
            static_cast<uint32_t>(algorithmIndex),
            static_cast<uint32_t>(kHome));
    }

    self->changingInternally = false;
    if (resend) sendProfile(self);
}

static void calculateRequirements(_NT_algorithmRequirements& req, const int32_t*) {
    req.numParameters = kNumParams;
    req.sram = sizeof(LoopyDial);
    req.dram = 0;
    req.dtc = 0;
    req.itc = 0;
}

static _NT_algorithm* construct(
    const _NT_algorithmMemoryPtrs& ptrs,
    const _NT_algorithmRequirements&,
    const int32_t*)
{
    LoopyDial* self = new (ptrs.sram) LoopyDial();
    std::memcpy(self->parameterDefs, kParameterDefaults, sizeof(kParameterDefaults));
    self->parameters = self->parameterDefs;
    self->parameterPages = &kParameterPages;
    return self;
}

static void parameterChanged(_NT_algorithm* algorithm, int parameter) {
    LoopyDial* self = static_cast<LoopyDial*>(algorithm);
    if (!self->ready || !self->v || self->changingInternally) return;

    switch (parameter) {
    case kProfile:
        if (self->feedbackProfile == self->v[kProfile]) {
            self->feedbackProfile = 0;
            return;
        }
        sendProfile(self);
        break;

    case kProfileCount:
        applyScaledRanges(self, true);
        break;

    default:
        break;
    }
}

static void step(_NT_algorithm* algorithm, float*, int) {
    LoopyDial* self = static_cast<LoopyDial*>(algorithm);
    if (self->ready || !self->v) return;

    int algorithmIndex = -1;
    if (!validAlgorithmIndex(self, algorithmIndex)) return;

    applyScaledRanges(self, false);
    self->ready = true;
}

static void midiMessage(
    _NT_algorithm* algorithm,
    uint8_t byte0,
    uint8_t byte1,
    uint8_t byte2)
{
    LoopyDial* self = static_cast<LoopyDial*>(algorithm);
    if (!self->ready || !self->v) return;

    const uint8_t expectedStatus = static_cast<uint8_t>(
        0xB0 | (clampInt(self->v[kMidiChannel], 1, 16) - 1));
    const uint8_t expectedCC = static_cast<uint8_t>(
        clampInt(self->v[kMidiCC], 0, 127));

    if (byte0 != expectedStatus || byte1 != expectedCC || byte2 > 127) return;

    const int count = clampInt(self->v[kProfileCount], 1, kMaxProfiles);
    const int profile = midiToProfile(byte2, count);
    if (profile == self->v[kProfile]) return;

    int algorithmIndex = -1;
    if (!validAlgorithmIndex(self, algorithmIndex)) return;

    self->feedbackProfile = static_cast<int16_t>(profile);
    NT_setParameterFromAudio(
        static_cast<uint32_t>(algorithmIndex),
        static_cast<uint32_t>(kProfile) + NT_parameterOffset(),
        static_cast<int16_t>(profile));
}

static int parameterString(_NT_algorithm*, int parameter, int value, char* buffer) {
    if (parameter != kProfile && parameter != kHome) return 0;

    value = clampInt(value, 1, kMaxProfiles);
    std::strcpy(buffer, "Profile ");
    const int len = 8 + NT_intToString(buffer + 8, value);
    return len;
}

static bool draw(_NT_algorithm* algorithm) {
    LoopyDial* self = static_cast<LoopyDial*>(algorithm);
    if (!self->v) {
        NT_drawText(4, 28, "Loopy Dial");
        return false;
    }

    const int count = clampInt(self->v[kProfileCount], 1, kMaxProfiles);
    const int profile = clampInt(self->v[kProfile], 1, count);

    char line[32];
    std::strcpy(line, "Profile ");
    int length = 8 + NT_intToString(line + 8, profile);
    line[length++] = '/';
    length += NT_intToString(line + length, count);
    line[length] = 0;

    NT_drawText(4, 28, line);
    return false;
}

static const _NT_factory kFactory = {
    .guid = NT_MULTICHAR('L','p','D','4'),
    .name = "Loopy Dial NT",
    .description = "Loopy Pro profile selector",
    .numSpecifications = 0,
    .specifications = NULL,
    .calculateStaticRequirements = NULL,
    .initialise = NULL,
    .calculateRequirements = calculateRequirements,
    .construct = construct,
    .parameterChanged = parameterChanged,
    .step = step,
    .draw = draw,
    .midiRealtime = NULL,
    .midiMessage = midiMessage,
    .tags = kNT_tagUtility,
    .hasCustomUi = NULL,
    .customUi = NULL,
    .setupUi = NULL,
    .serialise = NULL,
    .deserialise = NULL,
    .midiSysEx = NULL,
    .parameterUiPrefix = NULL,
    .parameterString = parameterString,
};

} // namespace

extern "C" uintptr_t pluginEntry(_NT_selector selector, uint32_t data) {
    switch (selector) {
    case kNT_selector_version:
        return kNT_apiVersionCurrent;
    case kNT_selector_numFactories:
        return 1;
    case kNT_selector_factoryInfo:
        return data == 0 ? reinterpret_cast<uintptr_t>(&kFactory) : 0;
    }
    return 0;
}
