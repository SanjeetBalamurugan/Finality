import argparse
import hashlib
import json
from pathlib import Path
import sys

engine = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(engine / '.tools/reflection'))
from clang import cindex
cindex.Config.libcxx_dir = None


def quote(value):
    return json.dumps(str(value).replace('\\', '/'))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--msvc', type=Path, required=True)
    parser.add_argument('--sdk', type=Path, required=True)
    parser.add_argument('--configuration', choices=['Debug', 'Release'], required=True)
    args = parser.parse_args()
    source = args.source.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    includes = [engine / 'FinalityEngine/src', engine / 'vendor/glm', engine / 'vendor/entt/include',
                engine / 'vendor/spdlog/include', engine / 'vendor/yaml-cpp/include', engine / 'vendor/imgui',
                args.msvc / 'include']
    includes.extend(args.sdk / part for part in ['ucrt', 'shared', 'um', 'winrt'])
    toolset = args.msvc.name.split('.')
    compatibility = f'{int(toolset[0]) + 5}.{toolset[1]}'
    flags = ['-x', 'c++', '-std=c++20', '--target=x86_64-pc-windows-msvc', '-fms-extensions',
             '-fms-compatibility', '-fms-compatibility-version=' + compatibility, '-Wno-ignored-attributes',
             '-D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH', '-DFINALITY_REFLECTION_SCAN',
             '-DGLM_FORCE_PURE', '-DGLM_FORCE_UNRESTRICTED_FLOAT', '-DIMGUI_API=__declspec(dllimport)',
             '-D_MT', '-D_DLL', '-DNDEBUG' if args.configuration == 'Release' else '-D_DEBUG']
    flags += ['-I' + str(include) for include in includes]
    if args.configuration == 'Debug':
        flags += ['-DDEBUG']
    index = cindex.Index.create()
    kind = cindex.CursorKind
    records = {kind.STRUCT_DECL, kind.CLASS_DECL}
    all_files = sorted(path for path in source.rglob('*') if path.suffix.lower() in {'.cpp', '.h', '.hpp'})
    cpp_files = [path for path in all_files if path.suffix.lower() == '.cpp']
    included_headers = set()
    units = []
    failures = []
    supported = set()

    def is_project(cursor):
        return cursor.location.file and Path(cursor.location.file.name).resolve().is_relative_to(source)

    def qualified(cursor):
        names = []
        while cursor and cursor.kind != kind.TRANSLATION_UNIT:
            if cursor.spelling:
                names.append(cursor.spelling)
            cursor = cursor.semantic_parent
        return '::' + '::'.join(reversed(names))

    def children(cursor):
        for child in cursor.get_children():
            if is_project(child):
                yield child
                if child.kind in records | {kind.NAMESPACE, kind.LINKAGE_SPEC, kind.UNEXPOSED_DECL}:
                    yield from children(child)

    def is_script(cursor, seen=None):
        if not cursor:
            return False
        if qualified(cursor) == '::FINALITY::ScriptableComponent':
            return True
        seen = set() if seen is None else seen
        key = cursor.get_usr()
        if key in seen:
            return False
        seen.add(key)
        return any(is_script(base.type.get_declaration(), seen) for base in cursor.get_children()
                   if base.kind == kind.CXX_BASE_SPECIFIER)

    def annotated(field):
        return any(child.kind == kind.ANNOTATE_ATTR and child.spelling == 'finality.serialize'
                   for child in field.get_children())

    def selected_fields(record):
        fields = []
        for field in record.get_children():
            if field.kind != kind.FIELD_DECL:
                continue
            if field.access_specifier != cindex.AccessSpecifier.PUBLIC and not annotated(field):
                continue
            if field.type.is_const_qualified():
                continue
            if field.is_bitfield() or field.type.kind in {cindex.TypeKind.LVALUEREFERENCE, cindex.TypeKind.RVALUEREFERENCE}:
                failures.append(f'{field.location}: field {field.spelling} cannot be serialized (bitfield or reference).')
                continue
            fields.append(field)
        return fields

    def public_bases(record):
        return [base.type.get_declaration() for base in record.get_children()
                if base.kind == kind.CXX_BASE_SPECIFIER and base.access_specifier == cindex.AccessSpecifier.PUBLIC
                and is_project(base.type.get_declaration())]

    def validate(field, stack):
        field_type = field.type.get_canonical()
        spelling = field_type.spelling.replace('class ', '').replace('struct ', '')
        if spelling in {'bool', 'int', 'float'} or field_type.spelling in supported:
            return
        declaration = field_type.get_declaration()
        if declaration.kind in records and is_project(declaration):
            key = declaration.get_usr()
            if key in stack:
                failures.append(f'{field.location}: recursive field {field.spelling} cannot be serialized.')
            else:
                for nested in selected_fields(declaration):
                    validate(nested, stack | {key})
            return
        failures.append(f'{field.location}: unsupported Inspector field {field.spelling}: {field.type.spelling}. '
                        'Supported: bool, int, float, std::string, glm::vec3, ScriptAssetReference and project structs containing these types.')

    def parse(path):
        content = path.read_text(encoding='utf-8-sig')
        content += '\nstruct FinalityReflectionBuiltinTypes { std::string String; glm::vec3 Vector; FINALITY::ScriptAssetReference Asset; };\n'
        translation = index.parse(str(path), args=flags + ['-include', 'Scene/ScriptModule.h'],
                                  unsaved_files=[(str(path), content)],
                                  options=cindex.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES)
        errors = [str(diagnostic) + '\n' + '\n'.join(str(note) for note in diagnostic.children) for diagnostic in translation.diagnostics
                  if diagnostic.severity >= cindex.Diagnostic.Error]
        if errors:
            raise RuntimeError('\n'.join(errors))
        for include in translation.get_includes():
            included_headers.add(Path(include.include.name).resolve())
        cursors = list(children(translation.cursor))
        for cursor in cursors:
            if cursor.spelling == 'FinalityReflectionBuiltinTypes':
                supported.update(field.type.get_canonical().spelling for field in cursor.get_children()
                                 if field.kind == kind.FIELD_DECL)
        definitions = [cursor for cursor in cursors if cursor.kind in records and cursor.is_definition()]
        scripts = [cursor for cursor in definitions if is_script(cursor) and not cursor.is_abstract_record()]
        legacy = any(cursor.kind == kind.FUNCTION_DECL and cursor.spelling == 'FinalityRegisterScripts' for cursor in cursors)
        needed = {}

        def collect(record):
            key = record.get_usr()
            if key in needed:
                return
            needed[key] = record
            for base in public_bases(record):
                collect(base)
            for field in selected_fields(record):
                validate(field, {record.get_usr()})
                nested = field.type.get_canonical().get_declaration()
                if nested.kind in records and is_project(nested):
                    collect(nested)

        for script in scripts:
            collect(script)
        units.append((path, list(needed.values()), scripts, legacy, translation))

    for path in cpp_files:
        parse(path)
    for path in all_files:
        if path.suffix.lower() != '.cpp' and path.resolve() not in included_headers:
            parse(path)
    if failures:
        raise RuntimeError('\n'.join(failures))
    outputs = {}
    legacy_functions = []
    for path, definitions, scripts, legacy, translation in units:
        key = hashlib.sha256(str(path.relative_to(source)).encode()).hexdigest()[:16]
        text = ['#include <Scene/ScriptModule.h>',
                f'#define FinalityScriptABI FinalityLegacyABI_{key}',
                f'#define FinalityRegisterScripts FinalityLegacyRegister_{key}',
                f'#include {quote(path)}', '#undef FinalityScriptABI', '#undef FinalityRegisterScripts',
                'namespace FINALITY {']
        ordered = []
        visited = set()

        def order(record):
            if record.get_usr() in visited:
                return
            visited.add(record.get_usr())
            for base in public_bases(record):
                order(base)
            ordered.append(record)

        for record in definitions:
            order(record)
        for record in ordered:
            if not record.spelling:
                continue
            fields = selected_fields(record)
            members = [f'MakeScriptField({quote(field.spelling)}, &{qualified(record)}::{field.spelling})' for field in fields]
            base_members = [f'ScriptFieldAccess::Members<{qualified(base)}>()' for base in public_bases(record)]
            text += [f'template<> inline auto ScriptFieldAccess::Members<{qualified(record)}>() {{',
                     'return std::tuple_cat(' + ', '.join(base_members + ['std::make_tuple(' + ', '.join(members) + ')']) + ');', '}']
        text += ['}', 'namespace {']
        for number, script in enumerate(scripts):
            text += [f'const bool FinalityReflected_{key}_{number} = FINALITY::ScriptReflection::Register<{qualified(script)}>({quote(qualified(script)[2:])}, {quote(script.location.file.name)});']
        text += ['}']
        if legacy:
            legacy_functions.append(f'FinalityLegacyRegister_{key}')
        outputs[f'Unit_{key}.cpp'] = '\n'.join(text) + '\n'
    registration = ['#include <Scene/ScriptModule.h>']
    registration += [f'extern "C" bool {name}(FINALITY::ScriptRegistrationContext*);' for name in legacy_functions]
    registration += ['extern "C" __declspec(dllexport) uint64_t FinalityScriptABI() { return FINALITY::ScriptModuleABI(); }',
                     'extern "C" __declspec(dllexport) bool FinalityRegisterScripts(FINALITY::ScriptRegistrationContext* context) {',
                     'if (!context) return false;',
                     'return FINALITY::ScriptReflection::RegisterModule([](const auto& merge) {',
                     'using Callback = std::function<bool(const FINALITY::ScriptDefinition&)>;',
                     'Callback callback = merge;',
                     'FINALITY::ScriptRegistrationContext legacyContext{&callback, [](void* user, const FINALITY::ScriptDefinition& definition) { return (*static_cast<Callback*>(user))(definition); }};']
    registration += [f'if (!{name}(&legacyContext)) return false;' for name in legacy_functions]
    registration += ['return true;', '}, [context](const auto& definition) { return context->Add(definition); });', '}']
    outputs['Registration.cpp'] = '\n'.join(registration) + '\n'
    for stale in args.output.glob('*.cpp'):
        if stale.name not in outputs:
            stale.unlink()
    for name, content in outputs.items():
        target = args.output / name
        if not target.exists() or target.read_text(encoding='utf-8') != content:
            target.write_text(content, encoding='utf-8')
    print(f'Reflected {sum(len(unit[2]) for unit in units)} script declarations in {len(units)} translation units.')


if __name__ == '__main__':
    try:
        main()
    except Exception as error:
        print(f'Native script reflection failed:\n{error}', file=sys.stderr)
        sys.exit(1)
