/** \file masp_extensions.cpp
    \author Mikko Kuitunen (mikko <dot> kuitunen <at> iki <dot> fi)
*/

#include "masp_extensions.h"
#include "masp_classwrap.h"
#include "iotools.h"

namespace masp{

masp::Value make_InputFile(Masp& m, Vector& args, Map& env){
    typedef WrappedObject<InputFile> WrappedInput;

    VecIterator arg_start = args.begin();
    VecIterator arg_end = args.end();

    std::string path;
    ArgWrap(arg_start, arg_end).wrap(&path);

    masp::Value obj = make_value_object(new WrappedInput(path.c_str()));

    FunMap fmap(m);
    fmap.add("is_open",            wrap_member(&InputFile::is_open));
    fmap.add("close",              wrap_member(&InputFile::close));
    fmap.add("contents_to_string", wrap_member(&InputFile::contents_to_string));

    return object_data_to_list(fmap, obj, m);
}

void outputfile_functions(masp::FunMap& fmap)
{
    fmap.add("is_open", wrap_member(&OutputFile::is_open));
    fmap.add("close",   wrap_member(&OutputFile::close));
    fmap.add("write",   wrap_member(&OutputFile::write));
}

masp::Value make_OutputFile(masp::Masp& m, masp::Vector& args, masp::Map& env){
    typedef WrappedObject<OutputFile> WrappedOutput;

    masp::VecIterator arg_start = args.begin();
    masp::VecIterator arg_end = args.end();

    std::string path;
    masp::ArgWrap(arg_start, arg_end).wrap(&path);

    masp::Value obj = masp::make_value_object(new WrappedOutput(path.c_str()));

    masp::FunMap fmap(m);
    outputfile_functions(fmap);

    return object_data_to_list(fmap, obj, m);
}

masp::Value make_OutputFileApp(masp::Masp& m, masp::Vector& args, masp::Map& env){
    typedef WrappedObject<OutputFile> WrappedOutput;

    masp::VecIterator arg_start = args.begin();
    masp::VecIterator arg_end = args.end();

    std::string path;
    bool        app;
    masp::ArgWrap(arg_start, arg_end).wrap(&path, &app);

    masp::Value obj = masp::make_value_object(new WrappedOutput(path.c_str(), app));

    masp::FunMap fmap(m);
    outputfile_functions(fmap);

    return object_data_to_list(fmap, obj, m);
}

void load_masp_unsafe_extensions(masp::Masp& m)
{
    masp::add_fun(m , "InputFile", make_InputFile);
    masp::add_fun(m , "OutputFile", make_OutputFile);
    masp::add_fun(m , "OutputFileApp", make_OutputFileApp);
}

}
