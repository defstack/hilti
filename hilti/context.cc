
#include <fstream>
#include <util/util.h>

#include <llvm/Support/DynamicLibrary.h>

#include "hilti-intern.h"
#include "parser/driver.h"
#include "autogen/hilti-config.h"
#include "jit/jit.h"

using namespace hilti;
using namespace hilti::passes;

CompilerContext::CompilerContext(const string_list libdirs)
{
    string_list paths = libdirs;

    for ( auto p : hilti::configuration().hilti_library_dirs )
        paths.push_back(p);

    paths.push_front(".");  // Always add cwd at the front.

    _libdirs = paths;
}

CompilerContext::~CompilerContext()
{
    delete _jit;
}

string CompilerContext::searchModule(shared_ptr<ID> id)
{
    auto p = util::strtolower(id->pathAsString());

    if ( ! util::endsWith(p, ".hlt") )
        p += ".hlt";

    string full_path = util::findInPaths(p, _libdirs);

    if ( full_path.size() == 0 ) {
        error(util::fmt("cannot find module %s", id->pathAsString()));
        goto error;
    }

    char buf[PATH_MAX];
    if ( ! realpath(full_path.c_str(), buf) ) {
        error(util::fmt("error reading %s: %s", full_path, strerror(errno)));
        goto error;
    }

    if ( debugging("context" ) )
        std::cerr << util::fmt("Found module %s as %s ...", id->pathAsString(), buf) << std::endl;

    return string(buf);

error:
    if ( debugging("context" ) )
        std::cerr << util::fmt("Did not find module %s ...", id->pathAsString()) << std::endl;

    return "";
}

bool CompilerContext::importModule(shared_ptr<ID> id)
{
    if ( debugging("context" ) )
        std::cerr << util::fmt("Importing module %s ...", id->pathAsString()) << std::endl;

    auto path = searchModule(id);

    if ( ! path.size() )
        return nullptr;

    return (loadModule(path, false, false) != nullptr);
}

shared_ptr<Module> CompilerContext::loadModule(const string& p, bool verify, bool finalize)
{
    if ( debugging("context" ) )
        std::cerr << util::fmt("Loading module %s ...", p) << std::endl;

    char buf[PATH_MAX];
    if ( ! realpath(p.c_str(), buf) ) {
        error(util::fmt("error reading %s: %s", p, strerror(errno)));
        return nullptr;
    }

    string path(buf);

    auto m = _modules.find(path);

    if ( m != _modules.end() )
        return m->second;

    std::ifstream in(path);

    if ( in.fail() ) {
        error(util::fmt("cannot open input file %s", path.c_str()));
        return nullptr;
    }

    auto module = parse(in, path);

    if ( ! module )
        return nullptr;

    _modules.insert(std::make_pair(path, module));

    if ( finalize ) {
        if ( ! CompilerContext::finalize(nullptr, verify) )
            return nullptr;
    }

    return module;
}

static void _debugAST(CompilerContext* ctx, shared_ptr<Module> module, const string& before)
{
    if ( ctx->debugging("dump-ast") ) {
        std::cerr << std::endl
            << "===" << std::endl
            << "=== AST for " << module->id()->pathAsString() << " before hilti::" << before << std::endl
            << "===" << std::endl
            << std::endl;

        ctx->dump(module, std::cerr);
    }

    if ( ctx->debugging("print-ast") ) {
        std::cerr << std::endl
            << "===" << std::endl
            << "=== AST for " << module->id()->pathAsString() << " before hilti::" << before << std::endl
            << "===" << std::endl
            << std::endl;

        ctx->print(module, std::cerr);
    }
}

static void _debugAST(CompilerContext* ctx, shared_ptr<Module> module, const ast::Logger& before)
{
    _debugAST(ctx, module, before.loggerName());
}

bool CompilerContext::_finalizeModule(shared_ptr<Module> module, bool verify)
{
    if ( debugging("context" ) )
        std::cerr << util::fmt("Finalizing module %s ...", module->id()->pathAsString()) << std::endl;

    // Just a double-check ...
    if ( ! InstructionRegistry::globalRegistry()->getAll().size() ) {
        internalError("hilti: no operators defined, did you call hilti::init()?");
    }

    passes::IdResolver          id_resolver;
    passes::InstructionResolver instruction_resolver;
    passes::BlockNormalizer     block_normalizer;
    passes::Validator           validator;

    _debugAST(this, module, instruction_resolver);

    if ( ! instruction_resolver.run(module) )
        return false;

    _debugAST(this, module, block_normalizer);

    if ( ! block_normalizer.run(module) )
        return false;

    // Run these again, we have inserted new instructions.

    _debugAST(this, module, id_resolver);

    if ( ! id_resolver.run(module) )
        return false;

    _debugAST(this, module, instruction_resolver);

    if ( ! instruction_resolver.run(module) )
        return false;

    _debugAST(this, module, validator);

    if ( verify && ! validator.run(module) )
        return false;

    auto cfg = std::make_shared<passes::CFG>(this);

    _debugAST(this, module, *cfg);

    if ( ! cfg->run(module) )
        return false;

    auto liveness = std::make_shared<passes::Liveness>(this, cfg);

    _debugAST(this, module, *liveness);

    if ( ! liveness->run(module) )
        return false;

    module->setPasses(cfg, liveness);

    return true;
}

void CompilerContext::addModule(shared_ptr<Module> module)
{
    if ( debugging("context" ) )
        std::cerr << util::fmt("Adding module %s ...", module->id()->pathAsString()) << std::endl;

    string path = module->path();

    if ( path == "-" || path == "" )
        path = util::fmt("<no path for %s>", module->id()->pathAsString());

    for ( auto m : _modules ) {
        if ( m.second == module )
            // Already added.
            return;
    }

    _modules.insert(std::make_pair(path, module));
}

bool CompilerContext::finalize(shared_ptr<Module> module, bool verify)
{
    if ( debugging("context" ) )
        std::cerr << util::fmt("Finalizing context...") << std::endl;

    if ( module )
        addModule(module);

    // This is implictly imported by all modules.
    importModule(std::make_shared<ID>("libhilti"));

    for ( auto m : _modules ) {
        auto module = m.second;

        passes::ScopeBuilder scope_builder(this);
        passes::ScopePrinter scope_printer(std::cerr);

        _debugAST(this, module, scope_builder);

        if ( ! scope_builder.run(module) )
            return false;

        if ( debugging("scopes") ) {
            _debugAST(this, module, scope_printer);
            scope_printer.run(module);
        }

    }

    for ( auto m : _modules ) {
        auto module = m.second;

        passes::IdResolver id_resolver;

        _debugAST(this, module, id_resolver);

        if ( ! id_resolver.run(module) )
            return false;
    }

    for ( auto m : _modules ) {
        auto module = m.second;

        if ( ! CompilerContext::_finalizeModule(module, verify) )
            goto error;
    }

    _modules.clear();
    return true;

error:
    _modules.clear();
    return false;
}

shared_ptr<Scope> CompilerContext::scopeAlias(shared_ptr<ID> id)
{
    if ( debugging("context" ) )
        std::cerr << util::fmt("Creating scope alias for module %s ...", id->pathAsString()) << std::endl;

    auto path = searchModule(id);

    if ( ! path.size() )
        return nullptr;

    auto m = _modules.find(path);

    if ( m == _modules.end() )
        internalError(util::fmt("unknown module %s in CompilerContext::scopeAlias", id->pathAsString()));

    auto module = m->second;
    return module->body()->scope()->createAlias();
}

shared_ptr<Module> CompilerContext::parse(std::istream& in, const std::string& sname)
{
    if ( debugging("context" ) )
        std::cerr << util::fmt("Parsing %s ...", sname) << std::endl;

    hilti_parser::Driver driver;
    driver.forwardLoggingTo(this);
    driver.enableDebug(debugging("scanner"), debugging("parser"));
    return driver.parse(shared_from_this(), in, sname);
}

llvm::Module* CompilerContext::compile(shared_ptr<Module> module, int debug, bool verify, int profile)
{
    if ( debugging("context" ) )
        std::cerr << util::fmt("Compiling module %s ...", module->id()->pathAsString()) << std::endl;

    _debugAST(this, module, "CodeGen");

    codegen::CodeGen cg(module->compilerContext()->libraryPaths());
    return cg.generateLLVM(module, verify, debug, profile);
}

bool CompilerContext::print(shared_ptr<Module> module, std::ostream& out, bool cfg)
{
    passes::Printer printer(out, false, cfg);
    return printer.run(module);
}

bool CompilerContext::printBitcode(llvm::Module* module, std::ostream& out)
{
    hilti::codegen::AssemblyAnnotationWriter annotator;
    llvm::raw_os_ostream llvm_out(out);
    module->print(llvm_out, &annotator);
    return true;
}

void CompilerContext::generatePrototypes(shared_ptr<Module> module, std::ostream& out)
{
    codegen::ProtoGen pg(out);
    return pg.generatePrototypes(module);
}

bool CompilerContext::dump(shared_ptr<Node> ast, std::ostream& out)
{
    ast->dump(out);
    return true;
}

bool CompilerContext::writeBitcode(llvm::Module* module, std::ostream& out)
{
    llvm::raw_os_ostream llvm_out(out);
    llvm::WriteBitcodeToFile(module, llvm_out);
    return true;
}

llvm::Module* CompilerContext::linkModules(string output, std::list<llvm::Module*> modules, path_list paths, std::list<string> libs, path_list bcas, path_list dylds, bool debug, bool verify, bool add_stdlibs, bool add_sharedlibs)
{
    if ( debugging("context" ) ) {
        std::list<string> names;

        for ( auto m : modules )
            names.push_back(m->getModuleIdentifier());

        std::cerr << util::fmt("Linking modules %s ...", util::strjoin(names, ", ")) << std::endl;
    }

    codegen::Linker linker(libs);

    for ( auto l : libs ) {
        if ( debugging("context" ) )
            std::cerr << "Linker: adding native library " << l << std::endl;

        linker.addNativeLibrary(l);
    }

    for ( auto b : bcas ) {
        if ( debugging("context" ) )
            std::cerr << "Linker: adding bitcode library " << b << std::endl;

        linker.addBitcodeArchive(b);
    }

    if ( add_stdlibs ) {
#if 0
        auto type_info = loadModule(configuration().runtime_typeinfo_hlt);

        if ( ! type_info )
            return nullptr;

        auto type_info_llvm = compile(type_info, debug);

        if ( ! type_info_llvm )
            return nullptr;

        modules.push_back(type_info_llvm);
#endif

        auto rlbca = debug ? configuration().runtime_library_bca_dbg : configuration().runtime_library_bca;

        if ( debugging("context" ) )
            std::cerr << "Linker: adding bitcode runtime library " << rlbca << std::endl;

        linker.addBitcodeArchive(rlbca);

        if ( debugging("context" ) )
            std::cerr << "Linker: adding native runtime library " << configuration().runtime_library_a << std::endl;

        linker.addNativeLibrary(configuration().runtime_library_a);
    }

    if ( add_sharedlibs ) {
        for ( auto d : configuration().runtime_shared_libraries )
            dylds.push_back(d);
    }

    for ( auto d : dylds ) {
        string errormsg;

        if ( ! util::startsWith(d, configuration().shared_library_prefix) )
            d = configuration().shared_library_prefix + d;

        if ( ! util::endsWith(d, configuration().shared_library_suffix) )
            d = d + configuration().shared_library_suffix;

        if ( debugging("context" ) )
            std::cerr << "Linker: adding dynamic library " << d << std::endl;

        // Sic. This returns true on error ...
        if ( llvm::sys::DynamicLibrary::LoadLibraryPermanently(d.c_str(), &errormsg) ) {
            error(errormsg);
            return nullptr;
        }
    }

    if ( debugging("context" ) ) {
        std::list<string> names;

        for ( auto m : modules )
            names.push_back(m->getModuleIdentifier());

        std::cerr << util::fmt("  Final set modules to link: %s ...", util::strjoin(names, ", ")) << std::endl;
    }

    return linker.link(output, modules, verify, debugging("linker"));
}

llvm::ExecutionEngine* CompilerContext::jitModule(llvm::Module* module, int optimize)
{
    if ( ! _jit )
        _jit = new jit::JIT(this);

    if ( debugging("context" ) )
        std::cerr << util::fmt("JITing module %s ...", module->getModuleIdentifier()) << std::endl;

    return _jit->jitModule(module, optimize);
}

void* CompilerContext::nativeFunction(llvm::Module* module, llvm::ExecutionEngine* ee, const string& function)
{
    if ( debugging("context" ) )
        std::cerr << util::fmt("Getting native function %s from module %s ...", function, module->getModuleIdentifier()) << std::endl;

    if ( ! _jit )
        _jit = new jit::JIT(this);

    return _jit->nativeFunction(ee, module, function);
}

void CompilerContext::installFunctionTable(const FunctionMapping* ftable)
{
    if ( debugging("context" ) )
        std::cerr << util::fmt("Installing custom function table ...") << std::endl;

    if ( ! _jit )
        _jit = new jit::JIT(this);

    _jit->installFunctionTable(ftable);
}

const string_list& CompilerContext::libraryPaths() const
{
    return _libdirs;
}

bool CompilerContext::debugging(const string& label)
{
    return _debug_streams.find(label) != _debug_streams.end();
}

bool CompilerContext::enableDebug(const string& label)
{
    if ( ! validDebugStream(label) )
        return false;

    _debug_streams.insert(label);

    if ( label == "visitors" )
        ast::enableDebuggingForAllVisitors();

    return true;
}

bool CompilerContext::enableDebug(std::set<string>& labels)
{
    for ( auto l : labels )
        enableDebug(l);

    return true;
}

std::list<string> CompilerContext::debugStreams()
{
    return { "codegen", "linker", "parser", "scanner", "scopes", "context", "dump-ast", "print-ast", "visitors" };
}

bool CompilerContext::validDebugStream(const string& label)
{
    auto streams = debugStreams();
    return std::find(streams.begin(), streams.end(), label) != streams.end();
}





