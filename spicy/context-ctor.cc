
// FIXME: Due to namespace issues, we can't have this in context.cc.

#include "spicy/autogen/spicy-config.h"

#include "context.h"
#include "jit.h"
#include "options.h"

#include <hilti/context.h>
#include <hilti/jit.h>

spicy::CompilerContext::CompilerContext(std::shared_ptr<Options> options)
{
    setOptions(options);
}

void spicy::CompilerContext::setOptions(std::shared_ptr<Options> options)
{
    _options = options;
    _hilti_context = std::make_shared<hilti::CompilerContextJIT<spicy::JIT>>(options);

    if ( options->cgDebugging("visitors") )
        ast::enableDebuggingForAllVisitors(true);
    else
        ast::enableDebuggingForAllVisitors(false);
}

std::unique_ptr<llvm::Module> spicy::CompilerContext::linkModules(
    string output, std::list<std::unique_ptr<llvm::Module>> modules, std::list<string> libs,
    path_list bcas, path_list dylds, bool add_stdlibs, bool add_sharedlibs)
{
    if ( add_stdlibs ) {
        if ( options().debug )
            bcas.push_back(configuration().runtime_library_bca_dbg);
        else
            bcas.push_back(configuration().runtime_library_bca);
    };

    return _hilti_context->linkModules(output, modules, libs, bcas, dylds, add_stdlibs,
                                       add_sharedlibs);
}

std::unique_ptr<llvm::Module> spicy::CompilerContext::linkModules(
    string output, std::list<std::unique_ptr<llvm::Module>> modules)
{
    return linkModules(output, std::move(modules), std::list<string>(), path_list(), path_list());
}
