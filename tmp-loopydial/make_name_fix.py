from pathlib import Path

p = Path('tmp-loopydial/loopydial.cpp')
s = p.read_text()

# Add normal NT parameters for profile-name editing. No custom encoder/push takeover.
s = s.replace(
    '    kHome,\n    kMidiChannel,',
    '    kHome,\n    kProfileName,\n    kNamePosition,\n    kNameCharacter,\n    kMidiChannel,')

char_strings = [' '] + list('ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_./')
char_cpp = ', '.join('"' + c.replace('\\', '\\\\').replace('"', '\\"') + '"' for c in char_strings)
s = s.replace(
    'static const int kNameLength = 32;\n',
    'static const int kNameLength = 32;\n'
    'static char const* const kNameCharacters[] = { ' + char_cpp + ' };\n')

s = s.replace(
    '    { .name = "Home",          .min = 1, .max = 32, .def = 1,   .unit = kNT_unitHasStrings, .scaling = kNT_scalingNone, .enumStrings = NULL },\n    { .name = "MIDI channel",',
    '    { .name = "Home",          .min = 1, .max = 32, .def = 1,   .unit = kNT_unitHasStrings, .scaling = kNT_scalingNone, .enumStrings = NULL },\n'
    '    { .name = "Profile name",  .min = 0, .max = 0,  .def = 0,   .unit = kNT_unitHasStrings, .scaling = kNT_scalingNone, .enumStrings = NULL },\n'
    '    { .name = "Name position", .min = 1, .max = 24, .def = 1,   .unit = kNT_unitNone,       .scaling = kNT_scalingNone, .enumStrings = NULL },\n'
    '    { .name = "Name character",.min = 0, .max = (int)ARRAY_SIZE(kNameCharacters)-1, .def = 0, .unit = kNT_unitEnum, .scaling = kNT_scalingNone, .enumStrings = kNameCharacters },\n'
    '    { .name = "MIDI channel",')

s = s.replace(
    'static const uint8_t kProfilePageParams[] = { kProfile, kProfileCount, kHome };',
    'static const uint8_t kProfilePageParams[] = { kProfile, kProfileCount, kHome, kProfileName, kNamePosition, kNameCharacter };')

insert_after = '''static void setDefaultNames(LoopyDial* self) {
    for (int i = 0; i < kMaxProfiles; ++i) {
        std::strcpy(self->names[i], "Profile ");
        NT_intToString(self->names[i] + 8, i + 1);
    }
    std::strcpy(self->names[0], "WitchboardX");
    std::strcpy(self->names[1], "FAC_Drumkit");
}
'''
helpers = r'''
static int nameCharacterIndex(char c) {
    for (int i = 0; i < (int)ARRAY_SIZE(kNameCharacters); ++i) {
        if (kNameCharacters[i][0] == c && kNameCharacters[i][1] == 0)
            return i;
    }
    return 0;
}

static void refreshNameDisplays(LoopyDial* self) {
    int algorithmIndex = NT_algorithmIndex(self);
    if (algorithmIndex < 0) return;
    NT_updateParameterDefinition((uint32_t)algorithmIndex, (uint32_t)kProfile);
    NT_updateParameterDefinition((uint32_t)algorithmIndex, (uint32_t)kHome);
    NT_updateParameterDefinition((uint32_t)algorithmIndex, (uint32_t)kProfileName);
}

static void syncNameCharacterParameter(LoopyDial* self) {
    if (!self->v) return;
    int algorithmIndex = NT_algorithmIndex(self);
    if (algorithmIndex < 0) return;
    const int count = clampInt(self->v[kProfileCount], 1, kMaxProfiles);
    const int profile = clampInt(self->v[kProfile], 1, count) - 1;
    const int pos = clampInt(self->v[kNamePosition], 1, kNameLength - 1) - 1;
    const int len = (int)std::strlen(self->names[profile]);
    const char c = pos < len ? self->names[profile][pos] : ' ';
    const int ci = nameCharacterIndex(c);
    self->changingInternally = true;
    NT_setParameterFromAudio((uint32_t)algorithmIndex,
        (uint32_t)kNameCharacter + NT_parameterOffset(), (int16_t)ci);
    self->changingInternally = false;
}

static void writeNameCharacter(LoopyDial* self) {
    if (!self->v) return;
    const int count = clampInt(self->v[kProfileCount], 1, kMaxProfiles);
    const int profile = clampInt(self->v[kProfile], 1, count) - 1;
    const int pos = clampInt(self->v[kNamePosition], 1, kNameLength - 1) - 1;
    const int ci = clampInt(self->v[kNameCharacter], 0, (int)ARRAY_SIZE(kNameCharacters) - 1);
    char* name = self->names[profile];
    int len = (int)std::strlen(name);
    while (len <= pos && len < kNameLength - 1) {
        name[len++] = ' ';
        name[len] = 0;
    }
    name[pos] = kNameCharacters[ci][0];
    if (pos >= len) name[pos + 1] = 0;
    len = (int)std::strlen(name);
    while (len > 0 && name[len - 1] == ' ') name[--len] = 0;
    if (len == 0) std::strcpy(name, "Profile");
    refreshNameDisplays(self);
}
'''
s = s.replace(insert_after, insert_after + helpers)

old_case = '''    case kProfile:
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
'''
new_case = '''    case kProfile:
        syncNameCharacterParameter(self);
        refreshNameDisplays(self);
        if (self->feedbackProfile == self->v[kProfile]) {
            self->feedbackProfile = 0;
            return;
        }
        sendProfile(self);
        break;
    case kProfileCount:
        applyScaledRanges(self, true);
        syncNameCharacterParameter(self);
        break;
    case kNamePosition:
        syncNameCharacterParameter(self);
        break;
    case kNameCharacter:
        writeNameCharacter(self);
        break;
    default:
        break;
'''
s = s.replace(old_case, new_case)

old_ps = '''static int parameterString(_NT_algorithm* algorithm, int parameter, int value, char* buffer) {
    if (parameter != kProfile && parameter != kHome) return 0;
    LoopyDial* self = static_cast<LoopyDial*>(algorithm);
    int count = kMaxProfiles;
    if (self->v) count = clampInt(self->v[kProfileCount], 1, kMaxProfiles);
    value = clampInt(value, 1, count);
    std::strncpy(buffer, self->names[value - 1], kNT_parameterStringSize - 1);
    buffer[kNT_parameterStringSize - 1] = 0;
    return static_cast<int>(std::strlen(buffer));
}
'''
new_ps = '''static int parameterString(_NT_algorithm* algorithm, int parameter, int value, char* buffer) {
    LoopyDial* self = static_cast<LoopyDial*>(algorithm);
    if (parameter == kProfileName) {
        int count = kMaxProfiles;
        int profile = 1;
        if (self->v) {
            count = clampInt(self->v[kProfileCount], 1, kMaxProfiles);
            profile = clampInt(self->v[kProfile], 1, count);
        }
        std::strncpy(buffer, self->names[profile - 1], kNT_parameterStringSize - 1);
        buffer[kNT_parameterStringSize - 1] = 0;
        return static_cast<int>(std::strlen(buffer));
    }
    if (parameter != kProfile && parameter != kHome) return 0;
    int count = kMaxProfiles;
    if (self->v) count = clampInt(self->v[kProfileCount], 1, kMaxProfiles);
    value = clampInt(value, 1, count);
    std::strncpy(buffer, self->names[value - 1], kNT_parameterStringSize - 1);
    buffer[kNT_parameterStringSize - 1] = 0;
    return static_cast<int>(std::strlen(buffer));
}
'''
s = s.replace(old_ps, new_ps)

# Do not override any knobs, encoders, or encoder pushes. Use the host's standard parameter UI.
start = s.index('static uint32_t hasCustomUi')
end = s.index('static bool newlyPressed', start)
s = s[:start] + '''static uint32_t hasCustomUi(_NT_algorithm*) {
    return 0;
}

''' + s[end:]

start = s.index('static void customUi')
end = s.index('static bool draw', start)
s = s[:start] + '''static void customUi(_NT_algorithm*, const _NT_uiData&) {
}

''' + s[end:]

# Keep the normal display, but remove instructions for custom controls.
s = s.replace(
    '    if (self->editingName)\n        NT_drawText(4, 55, "B2 done  L char  R cursor", 12, kNT_textLeft, kNT_textTiny);\n    else\n        NT_drawText(4, 55, "L select/push Home  B2 rename", 12, kNT_textLeft, kNT_textTiny);',
    '    NT_drawText(4, 55, "Edit Name position + Name character", 12, kNT_textLeft, kNT_textTiny);')

p.write_text(s)
