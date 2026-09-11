#include <new>
#include <string.h>
#include <distingnt/api.h>
#include <distingnt/serialisation.h>

namespace {

enum ParamIndex {
    kParamProfile = 0,
    kParamProfileCount,
    kParamHome,
    kParamMidiChannel,
    kParamMidiCC,
    kNumParams
};

static const int kMaxProfiles = 32;
static const int kProfileNameLength = 24;
static const uint32_t kMidiDestination = kNT_destinationUSB;

static const _NT_parameter kParameterDefaults[kNumParams] = {
    { "Profile",       1, 32,  1, kNT_unitHasStrings, kNT_scalingNone, NULL },
    { "Profile count", 1, 32,  2, kNT_unitNone,       kNT_scalingNone, NULL },
    { "Home",          1, 32,  1, kNT_unitHasStrings, kNT_scalingNone, NULL },
    { "MIDI channel",  1, 16,  1, kNT_unitNone,       kNT_scalingNone, NULL },
    { "MIDI CC",       0, 127, 119, kNT_unitNone,     kNT_scalingNone, NULL },
};

static const uint8_t kProfilePageParams[] = {
    kParamProfile,
    kParamProfileCount,
    kParamHome
};

static const uint8_t kMidiPageParams[] = {
    kParamMidiChannel,
    kParamMidiCC
};

static const _NT_parameterPage kPages[] = {
    { "Profile", 3, 0, { 0, 0 }, kProfilePageParams },
    { "MIDI",    2, 0, { 0, 0 }, kMidiPageParams },
};

static const _NT_parameterPages kParameterPages = {
    2,
    kPages
};

struct LoopyProfileSelector : public _NT_algorithm {
    LoopyProfileSelector()
        : registered(false),
          updatingRanges(false),
          pendingFeedbackProfile(0)
    {
    }

    _NT_parameter params[kNumParams];
    bool registered;
    bool updatingRanges;
    int16_t pendingFeedbackProfile;
    char profileNames[kMaxProfiles][kProfileNameLength];
};

static int clampInt(int value, int low, int high)
{
    if (value < low)
        return low;
    if (value > high)
        return high;
    return value;
}

static void copyText(char* dst, int capacity, const char* src)
{
    if (capacity <= 0)
        return;
    if (!src)
        src = "";

    int i = 0;
    for (; i < capacity - 1 && src[i]; ++i)
        dst[i] = src[i];
    dst[i] = '\0';
}

static int appendInt(char* dst, int capacity, int value)
{
    if (capacity <= 0)
        return 0;

    char tmp[12];
    int count = 0;
    do {
        tmp[count++] = static_cast<char>('0' + (value % 10));
        value /= 10;
    } while (value > 0 && count < static_cast<int>(sizeof(tmp)));

    int written = 0;
    while (count > 0 && written < capacity - 1)
        dst[written++] = tmp[--count];
    dst[written] = '\0';
    return written;
}

static void setDefaultProfileName(char* dst, int profile)
{
    copyText(dst, kProfileNameLength, "Profile ");
    appendInt(dst + strlen(dst), kProfileNameLength - strlen(dst), profile);
}

static void setDefaultProfileNames(LoopyProfileSelector* self)
{
    for (int i = 0; i < kMaxProfiles; ++i)
        setDefaultProfileName(self->profileNames[i], i + 1);
}

static int encodeProfile(int index, int count)
{
    if (count <= 1)
        return 0;

    const int d = count - 1;
    index = clampInt(index, 0, d);

    return clampInt((128 * index + d / 2) / d, 0, 127);
}

static int decodeProfile(int value, int count)
{
    value = clampInt(value, 0, 127);
    count = clampInt(count, 1, kMaxProfiles);

    if (count <= 1)
        return 1;

    int bestIndex = 0;
    int bestDistance = 128;

    for (int i = 0; i < count; ++i) {
        int distance = encodeProfile(i, count) - value;
        if (distance < 0)
            distance = -distance;

        if (distance < bestDistance) {
            bestDistance = distance;
            bestIndex = i;
        }
    }

    return bestIndex + 1;
}

static int currentProfileCount(const LoopyProfileSelector* self)
{
    return clampInt(self->v[kParamProfileCount], 1, kMaxProfiles);
}

static void sendProfile(const LoopyProfileSelector* self)
{
    const int count = currentProfileCount(self);
    const int profile = clampInt(self->v[kParamProfile], 1, count);
    const int channel = clampInt(self->v[kParamMidiChannel], 1, 16);
    const int cc = clampInt(self->v[kParamMidiCC], 0, 127);
    const int encoded = encodeProfile(profile - 1, count);

    NT_sendMidi3ByteMessage(
        kMidiDestination,
        static_cast<uint8_t>(0xB0 | (channel - 1)),
        static_cast<uint8_t>(cc),
        static_cast<uint8_t>(encoded));
}

static bool algorithmIndex(const _NT_algorithm* self, uint32_t& index)
{
    const int32_t i = NT_algorithmIndex(self);
    if (i < 0)
        return false;

    index = static_cast<uint32_t>(i);
    return true;
}

static void setParameterIfNeeded(
    LoopyProfileSelector* self,
    uint32_t algorithm,
    ParamIndex parameter,
    int value)
{
    if (self->v[parameter] == value)
        return;

    NT_setParameterFromAudio(
        algorithm,
        static_cast<uint32_t>(parameter) + NT_parameterOffset(),
        static_cast<int16_t>(value));
}

static void updateProfileRanges(LoopyProfileSelector* self)
{
    uint32_t algorithm = 0;
    if (!algorithmIndex(self, algorithm))
        return;

    const int count = currentProfileCount(self);

    self->updatingRanges = true;

    if (self->params[kParamProfile].max != count) {
        self->params[kParamProfile].max = static_cast<int16_t>(count);
        NT_updateParameterDefinition(algorithm, kParamProfile);
    }

    if (self->params[kParamHome].max != count) {
        self->params[kParamHome].max = static_cast<int16_t>(count);
        NT_updateParameterDefinition(algorithm, kParamHome);
    }

    setParameterIfNeeded(self, algorithm, kParamProfile, clampInt(self->v[kParamProfile], 1, count));
    setParameterIfNeeded(self, algorithm, kParamHome, clampInt(self->v[kParamHome], 1, count));

    self->updatingRanges = false;
}

static void calculateRequirements(_NT_algorithmRequirements& req, const int32_t* specifications)
{
    (void)specifications;

    req.numParameters = kNumParams;
    req.sram = sizeof(LoopyProfileSelector);
    req.dram = 0;
    req.dtc = 0;
    req.itc = 0;
}

static _NT_algorithm* construct(
    const _NT_algorithmMemoryPtrs& ptrs,
    const _NT_algorithmRequirements& req,
    const int32_t* specifications)
{
    (void)req;
    (void)specifications;

    LoopyProfileSelector* self = new (ptrs.sram) LoopyProfileSelector();
    memcpy(self->params, kParameterDefaults, sizeof(kParameterDefaults));
    setDefaultProfileNames(self);

    self->parameters = self->params;
    self->parameterPages = &kParameterPages;

    return self;
}

static void parameterChanged(_NT_algorithm* algorithm, int parameter)
{
    LoopyProfileSelector* self = static_cast<LoopyProfileSelector*>(algorithm);

    if (!self->v || !self->registered || self->updatingRanges)
        return;

    switch (parameter) {
    case kParamProfile:
        if (self->pendingFeedbackProfile == self->v[kParamProfile]) {
            self->pendingFeedbackProfile = 0;
            return;
        }
        sendProfile(self);
        break;

    case kParamProfileCount:
        updateProfileRanges(self);
        sendProfile(self);
        break;

    default:
        break;
    }
}

static void step(_NT_algorithm* algorithm, float* busFrames, int numFramesBy4)
{
    (void)busFrames;
    (void)numFramesBy4;

    LoopyProfileSelector* self = static_cast<LoopyProfileSelector*>(algorithm);
    if (!self->v || self->registered)
        return;

    updateProfileRanges(self);
    self->registered = true;
}

static void midiMessage(
    _NT_algorithm* algorithm,
    uint8_t byte0,
    uint8_t byte1,
    uint8_t byte2)
{
    LoopyProfileSelector* self = static_cast<LoopyProfileSelector*>(algorithm);
    if (!self->v || !self->registered)
        return;

    const uint8_t status = static_cast<uint8_t>(byte0 & 0xF0);
    const uint8_t channel = static_cast<uint8_t>(byte0 & 0x0F);
    const int configuredChannel = clampInt(self->v[kParamMidiChannel], 1, 16) - 1;
    const int configuredCC = clampInt(self->v[kParamMidiCC], 0, 127);

    if (status != 0xB0)
        return;
    if (channel != configuredChannel)
        return;
    if (byte1 != configuredCC)
        return;
    if (byte2 > 127)
        return;

    const int profile = decodeProfile(byte2, currentProfileCount(self));
    if (profile == self->v[kParamProfile])
        return;

    uint32_t algorithmIndexValue = 0;
    if (!algorithmIndex(self, algorithmIndexValue))
        return;

    self->pendingFeedbackProfile = static_cast<int16_t>(profile);
    NT_setParameterFromAudio(
        algorithmIndexValue,
        static_cast<uint32_t>(kParamProfile) + NT_parameterOffset(),
        static_cast<int16_t>(profile));
}

static int parameterString(_NT_algorithm* self, int parameter, int value, char* buffer)
{
    if (parameter != kParamProfile && parameter != kParamHome)
        return 0;

    LoopyProfileSelector* selector = static_cast<LoopyProfileSelector*>(self);
    const int profile = clampInt(value, 1, kMaxProfiles);
    copyText(buffer, kNT_parameterStringSize, selector->profileNames[profile - 1]);
    return strlen(buffer);
}

static void serialise(_NT_algorithm* algorithm, _NT_jsonStream& stream)
{
    LoopyProfileSelector* self = static_cast<LoopyProfileSelector*>(algorithm);

    stream.addMemberName("dialNames");
    stream.openArray();
    for (int i = 0; i < kMaxProfiles; ++i)
        stream.addString(self->profileNames[i]);
    stream.closeArray();
}

static bool deserialise(_NT_algorithm* algorithm, _NT_jsonParse& parse)
{
    LoopyProfileSelector* self = static_cast<LoopyProfileSelector*>(algorithm);
    setDefaultProfileNames(self);

    int members = 0;
    if (!parse.numberOfObjectMembers(members))
        return false;

    for (int member = 0; member < members; ++member) {
        if (!parse.matchName("dialNames")) {
            if (!parse.skipMember())
                return false;
            continue;
        }

        int count = 0;
        if (!parse.numberOfArrayElements(count))
            return false;

        for (int i = 0; i < count; ++i) {
            const char* name = NULL;
            if (!parse.string(name))
                return false;
            if (i < kMaxProfiles)
                copyText(self->profileNames[i], kProfileNameLength, name);
        }
    }

    return true;
}

static const _NT_factory kFactory = {
    NT_MULTICHAR('L', 'P', 'r', 'f'),
    "Dial",
    "Loopy Pro control profile selector",
    0,
    NULL,
    NULL,
    NULL,
    calculateRequirements,
    construct,
    parameterChanged,
    step,
    NULL,
    NULL,
    midiMessage,
    kNT_tagUtility,
    NULL,
    NULL,
    NULL,
    serialise,
    deserialise,
    NULL,
    NULL,
    parameterString
};

} // namespace

extern "C" uintptr_t pluginEntry(_NT_selector selector, uint32_t data)
{
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
