from pathlib import Path

p = Path('tmp-loopydial/loopydial.cpp')
s = p.read_text()

s = s.replace('    kHome,\n    kMidiChannel,', '    kHome,\n    kRename,\n    kMidiChannel,')
s = s.replace('static const int kNameLength = 32;\n', 'static const int kNameLength = 32;\nstatic char const* const kRenameStrings[] = { "Off", "Edit" };\n')
s = s.replace(
    '    { .name = "Home",          .min = 1, .max = 32, .def = 1,   .unit = kNT_unitHasStrings, .scaling = kNT_scalingNone, .enumStrings = NULL },\n    { .name = "MIDI channel",',
    '    { .name = "Home",          .min = 1, .max = 32, .def = 1,   .unit = kNT_unitHasStrings, .scaling = kNT_scalingNone, .enumStrings = NULL },\n    { .name = "Rename profile",.min = 0, .max = 1,  .def = 0,   .unit = kNT_unitEnum,       .scaling = kNT_scalingNone, .enumStrings = kRenameStrings },\n    { .name = "MIDI channel",')
s = s.replace(
    'static const uint8_t kProfilePageParams[] = { kProfile, kProfileCount, kHome };',
    'static const uint8_t kProfilePageParams[] = { kProfile, kProfileCount, kHome, kRename };')
s = s.replace(
    '    case kProfileCount:\n        applyScaledRanges(self, true);\n        break;\n\n    default:',
    '    case kProfileCount:\n        applyScaledRanges(self, true);\n        break;\n\n    case kRename:\n        if (self->v[kRename] != 0) {\n            self->editingName = true;\n            self->nameCursor = 0;\n        }\n        break;\n\n    default:')

start = s.index('static uint32_t hasCustomUi')
end = s.index('static bool newlyPressed', start)
s = s[:start] + '''static uint32_t hasCustomUi(_NT_algorithm* algorithm) {
    LoopyDial* self = static_cast<LoopyDial*>(algorithm);
    if (!self->editingName)
        return 0;
    return kNT_encoderL | kNT_encoderR | kNT_encoderButtonR | kNT_button2 | kNT_button3;
}

''' + s[end:]

start = s.index('static void customUi')
end = s.index('static bool draw', start)
s = s[:start] + '''static void customUi(_NT_algorithm* algorithm, const _NT_uiData& data) {
    LoopyDial* self = static_cast<LoopyDial*>(algorithm);
    if (!self->editingName || !self->ready || !self->v)
        return;

    int algorithmIndex = -1;
    if (!validAlgorithmIndex(self, algorithmIndex))
        return;

    const int count = clampInt(self->v[kProfileCount], 1, kMaxProfiles);
    const int profile = clampInt(self->v[kProfile], 1, count) - 1;
    char* name = self->names[profile];
    bool changed = false;

    // During rename only: left encoder = cursor, right encoder = character.
    if (data.encoders[0] != 0) {
        self->nameCursor = static_cast<uint8_t>(clampInt(
            static_cast<int>(self->nameCursor) + (data.encoders[0] > 0 ? 1 : -1),
            0, kNameLength - 2));
    }

    if (data.encoders[1] != 0) {
        int length = static_cast<int>(std::strlen(name));
        while (length <= self->nameCursor && length < kNameLength - 1) {
            name[length++] = ' ';
            name[length] = 0;
        }
        int c = static_cast<unsigned char>(name[self->nameCursor]);
        if (c < 32 || c > 126) c = 32;
        c += data.encoders[1] > 0 ? 1 : -1;
        if (c > 126) c = 32;
        if (c < 32) c = 126;
        name[self->nameCursor] = static_cast<char>(c);
        changed = true;
    }

    // Match the NT text-editing idea: insert and delete are explicit buttons.
    if (newlyPressed(kNT_button2, data)) {
        int length = static_cast<int>(std::strlen(name));
        if (length < kNameLength - 1) {
            const int pos = clampInt(self->nameCursor, 0, length);
            for (int i = length; i > pos; --i) name[i] = name[i - 1];
            name[pos] = ' ';
            name[length + 1] = 0;
            changed = true;
        }
    }
    if (newlyPressed(kNT_button3, data)) {
        const int length = static_cast<int>(std::strlen(name));
        const int pos = static_cast<int>(self->nameCursor);
        if (pos < length) {
            for (int i = pos; i < length; ++i) name[i] = name[i + 1];
            changed = true;
        }
    }

    // Right encoder push finishes rename and gives all controls back to the host.
    if (newlyPressed(kNT_encoderButtonR, data)) {
        self->editingName = false;
        self->changingInternally = true;
        NT_setParameterFromUi(
            static_cast<uint32_t>(algorithmIndex),
            static_cast<uint32_t>(kRename) + NT_parameterOffset(),
            0);
        self->changingInternally = false;
    }

    if (changed) {
        NT_updateParameterDefinition(static_cast<uint32_t>(algorithmIndex), static_cast<uint32_t>(kProfile));
        NT_updateParameterDefinition(static_cast<uint32_t>(algorithmIndex), static_cast<uint32_t>(kHome));
    }
}

''' + s[end:]

s = s.replace(
    '    if (self->editingName)\n        NT_drawText(4, 55, "B2 done  L char  R cursor", 12, kNT_textLeft, kNT_textTiny);\n    else\n        NT_drawText(4, 55, "L select/push Home  B2 rename", 12, kNT_textLeft, kNT_textTiny);',
    '    if (self->editingName)\n        NT_drawText(4, 55, "L cursor R char  B2 ins B3 del  R push done", 12, kNT_textLeft, kNT_textTiny);\n    else\n        NT_drawText(4, 55, "Set Rename profile to Edit", 12, kNT_textLeft, kNT_textTiny);')

p.write_text(s)
