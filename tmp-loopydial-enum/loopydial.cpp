#include <new>
#include <cstring>
#include <distingnt/api.h>
#include <distingnt/serialisation.h>

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
static const int kNameLength = 32;

static const _NT_parameter kParameterDefaults[kNumParams] = {
    { .name = "Profile",       .min = 1, .max = 32, .def = 1,   .unit = kNT_unitEnum, .scaling = kNT_scalingNone, .enumStrings = NULL },
    { .name = "Profile count", .min = 1, .max = 32, .def = 2,   .unit = kNT_unitNone, .scaling = kNT_scalingNone, .enumStrings = NULL },
    { .name = "Home",          .min = 1, .max = 32, .def = 1,   .unit = kNT_unitEnum, .scaling = kNT_scalingNone, .enumStrings = NULL },
    { .name = "MIDI channel",  .min = 1, .max = 16, .def = 1,   .unit = kNT_unitNone, .scaling = kNT_scalingNone, .enumStrings = NULL },
    { .name = "MIDI CC",       .min = 0, .max = 127,.def = 119, .unit = kNT_unitNone, .scaling = kNT_scalingNone, .enumStrings = NULL },
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
    LoopyDial()
    : ready(false), changingInternally(false), editingName(false),
      nameCursor(0), feedbackProfile(0) {}

    _NT_parameter parameterDefs[kNumParams];
    char names[kMaxProfiles][kNameLength];
    const char* enumNames[kMaxProfiles + 1];
    bool ready;
    bool changingInternally;
    bool editingName;
    uint8_t nameCursor;
    int16_t feedbackProfile;
};

static int clampInt(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static int profileToMidi(int zeroBasedIndex, int count) {
    count = clampInt(count, 1, kMaxProfiles);
    if (count <= 1) return 0;
    zeroBasedIndex = clampInt(zeroBasedIndex, 0, count - 1);
    const int denominator = count - 1;
    int value = (128 * zeroBasedIndex + denominator / 2) / denominator;
    return value > 127 ? 127 : value;
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

static void setDefaultNames(LoopyDial* self) {
    for (int i = 0; i < kMaxProfiles; ++i) {
        std::strcpy(self->names[i], "Profile ");
        NT_intToString(self->names[i] + 8, i + 1);
    }
    std::strcpy(self->names[0], "WitchboardX");
    std::strcpy(self->names[1], "FAC_Drumkit");
}

static void bindEnumNames(LoopyDial* self) {
    // NT enum values are 1..N here, so index 0 is deliberately unused.
    // This matches official API examples with enum min=1.
    self->enumNames[0] = "";
    for (int i = 0; i < kMaxProfiles; ++i)
        self->enumNames[i + 1] = self->names[i];

    self->parameterDefs[kProfile].enumStrings = self->enumNames;
    self->parameterDefs[kHome].enumStrings = self->enumNames;
}

static bool validAlgorithmIndex(_NT_algorithm* self, int& index) {
    index = NT_algorithmIndex(self);
    return index >= 0;
}

static void refreshNamedParameters(LoopyDial* self) {
    int algorithmIndex = -1;
    if (!validAlgorithmIndex(self, algorithmIndex)) return;

    NT_updateParameterDefinition(
        static_cast<uint32_t>(algorithmIndex),
        static_cast<uint32_t>(kProfile));
    NT_updateParameterDefinition(
        static_cast<uint32_t>(algorithmIndex),
        static_cast<uint32_t>(kHome));
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

static void applyScaledRanges(LoopyDial* self, bool resend, bool forceRefresh) {
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

    bool profileChanged = self->parameterDefs[kProfile].max != count;
    bool homeChanged = self->parameterDefs[kHome].max != count;

    self->parameterDefs[kProfile].max = static_cast<int16_t>(count);
    self->parameterDefs[kHome].max = static_cast<int16_t>(count);

    if (profileChanged || forceRefresh) {
        NT_updateParameterDefinition(
            static_cast<uint32_t>(algorithmIndex),
            static_cast<uint32_t>(kProfile));
    }
    if (homeChanged || forceRefresh) {
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
    setDefaultNames(self);
    bindEnumNames(self);
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
        applyScaledRanges(self, true, false);
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

    applyScaledRanges(self, false, true);
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

static uint32_t hasCustomUi(_NT_algorithm* algorithm) {
    LoopyDial* self = static_cast<LoopyDial*>(algorithm);
    if (self->editingName)
        return kNT_button2 | kNT_encoderL | kNT_encoderR;
    return kNT_button2 | kNT_encoderL | kNT_encoderButtonL;
}

static bool newlyPressed(uint16_t mask, const _NT_uiData& data) {
    return (data.controls & mask) && !(data.lastButtons & mask);
}

static void customUi(_NT_algorithm* algorithm, const _NT_uiData& data) {
    LoopyDial* self = static_cast<LoopyDial*>(algorithm);
    if (!self->ready || !self->v) return;

    int algorithmIndex = -1;
    if (!validAlgorithmIndex(self, algorithmIndex)) return;

    if (newlyPressed(kNT_button2, data)) {
        self->editingName = !self->editingName;
        self->nameCursor = 0;
        return;
    }

    if (!self->editingName) {
        if (newlyPressed(kNT_encoderButtonL, data)) {
            const int count = clampInt(self->v[kProfileCount], 1, kMaxProfiles);
            const int home = clampInt(self->v[kHome], 1, count);
            NT_setParameterFromUi(
                static_cast<uint32_t>(algorithmIndex),
                static_cast<uint32_t>(kProfile) + NT_parameterOffset(),
                static_cast<int16_t>(home));
            return;
        }

        if (data.encoders[0] != 0) {
            const int count = clampInt(self->v[kProfileCount], 1, kMaxProfiles);
            const int current = clampInt(self->v[kProfile], 1, count);
            const int next = clampInt(
                current + (data.encoders[0] > 0 ? 1 : -1), 1, count);

            if (next != current) {
                NT_setParameterFromUi(
                    static_cast<uint32_t>(algorithmIndex),
                    static_cast<uint32_t>(kProfile) + NT_parameterOffset(),
                    static_cast<int16_t>(next));
            }
        }
        return;
    }

    const int count = clampInt(self->v[kProfileCount], 1, kMaxProfiles);
    const int profile = clampInt(self->v[kProfile], 1, count) - 1;
    char* name = self->names[profile];
    bool nameChanged = false;

    if (data.encoders[1] != 0) {
        const int cursor = clampInt(
            static_cast<int>(self->nameCursor) + (data.encoders[1] > 0 ? 1 : -1),
            0, kNameLength - 2);
        self->nameCursor = static_cast<uint8_t>(cursor);
    }

    if (data.encoders[0] != 0) {
        int length = static_cast<int>(std::strlen(name));
        while (length <= self->nameCursor && length < kNameLength - 1) {
            name[length++] = ' ';
            name[length] = 0;
        }

        int c = static_cast<unsigned char>(name[self->nameCursor]);
        if (c < 32 || c > 126) c = 32;
        c += data.encoders[0] > 0 ? 1 : -1;
        if (c > 126) c = 32;
        if (c < 32) c = 126;
        name[self->nameCursor] = static_cast<char>(c);
        nameChanged = true;
    }

    if (nameChanged)
        refreshNamedParameters(self);
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

    NT_drawText(4, 24, line);
    NT_drawText(4, 39, self->names[profile - 1]);

    if (self->editingName)
        NT_drawText(4, 55, "B2 done  L char  R cursor", 12, kNT_textLeft, kNT_textTiny);
    else
        NT_drawText(4, 55, "L select/push Home  B2 rename", 12, kNT_textLeft, kNT_textTiny);

    return false;
}

static void serialise(_NT_algorithm* algorithm, _NT_jsonStream& stream) {
    LoopyDial* self = static_cast<LoopyDial*>(algorithm);
    stream.addMemberName("loopyDialNames");
    stream.openArray();
    for (int i = 0; i < kMaxProfiles; ++i)
        stream.addString(self->names[i]);
    stream.closeArray();
}

static bool deserialise(_NT_algorithm* algorithm, _NT_jsonParse& parse) {
    LoopyDial* self = static_cast<LoopyDial*>(algorithm);
    int members = 0;
    if (!parse.numberOfObjectMembers(members)) return false;

    for (int i = 0; i < members; ++i) {
        if (parse.matchName("loopyDialNames")) {
            int count = 0;
            if (!parse.numberOfArrayElements(count)) return false;

            for (int n = 0; n < count; ++n) {
                const char* source = NULL;
                if (!parse.string(source) || !source) return false;

                if (n < kMaxProfiles) {
                    std::strncpy(self->names[n], source, kNameLength - 1);
                    self->names[n][kNameLength - 1] = 0;
                }
            }
        } else if (!parse.skipMember()) {
            return false;
        }
    }

    bindEnumNames(self);
    self->ready = false;
    self->changingInternally = false;
    self->editingName = false;
    self->nameCursor = 0;
    self->feedbackProfile = 0;
    return true;
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
    .hasCustomUi = hasCustomUi,
    .customUi = customUi,
    .setupUi = NULL,
    .serialise = serialise,
    .deserialise = deserialise,
    .midiSysEx = NULL,
    .parameterUiPrefix = NULL,
    .parameterString = NULL,
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
