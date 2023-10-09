
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "CompoundFs/Composite.h"
#include <CompoundFs/WindowsFile.h>
#include <CompoundFs/Path.h>
#include <sstream>

using namespace TxFs;
namespace py = pybind11;

namespace
{
FileSystem openCompoundFile(const char* path, OpenMode mode)
{
    if (mode == OpenMode::ReadOnly)
        return Composite::openReadOnly<WindowsFile>(path, mode);
    else
        return Composite::open<WindowsFile>(path, mode);
}

std::string valueToString(const TreeValue& tv)
{
    std::ostringstream ss;
    switch (tv.getType())
    {
    case TreeValue::Type::Double:
        ss << tv.get<double>();
        break;
    case TreeValue::Type::File:
        ss << tv.get<FileDescriptor>().m_fileSize;
        break;
    case TreeValue::Type::Int32:
        ss << tv.get<uint32_t>();
        break;
    case TreeValue::Type::Int64:
        ss << tv.get<uint64_t>();
        break;
    case TreeValue::Type::String:
        ss << tv.get<std::string>();
        break;
    case TreeValue::Type::Version: {
        auto v = tv.get<Version>();
        ss << v.m_major << "." << v.m_minor << "." << v.m_minor;
        break;
    }
    default:
        break;
    }

    return ss.str();
}

std::string strImpl(const TreeValue& tv)
{
    std::ostringstream ss;
    ss << tv.getTypeName();
    for (int i = 0; i < (10 - tv.getTypeName().size()); i++)
        ss << " ";
    ss << valueToString(tv);
    return ss.str();
}

std::string reprImpl(const TreeValue& tv)
{
    std::ostringstream ss;
    ss << "<Attribute(" << tv.getTypeName() << "): " << valueToString(tv) << ">";
    return ss.str();
}

}

PYBIND11_MODULE(py_txfs, m)
{
    py::class_<WriteHandle>(m, "WriteHandle").def("__repr__", [](WriteHandle wh) {
        std::ostringstream ss;
        ss << "<WriteHandle: " << int(wh) << '>';
        return ss.str();
    });
    py::class_<ReadHandle>(m, "ReadHandle").def("__repr__", [](ReadHandle wh) {
        std::ostringstream ss;
        ss << "<ReadHandle: " << int(wh) << '>';
        return ss.str();
    });

    py::class_<TreeValue>(m, "Attribute").def("__str__", &strImpl).def("__repr__", &reprImpl);

    py::class_<FileSystem>(m, "FileSystem")
        .def("createFile", [](FileSystem& fs, const char* path) { return fs.createFile(path); })
        .def("appendFile", [](FileSystem& fs, const char* path) { return fs.appendFile(path); })
        .def("readFile", [](FileSystem& fs, const char* path) { return fs.readFile(path); })
        .def("fileSize", [](const FileSystem& fs, const char* path) { return fs.fileSize(path); })
        .def("fileSize", [](const FileSystem& fs, WriteHandle h) { return fs.fileSize(h); })
        .def("fileSize", [](const FileSystem& fs, ReadHandle h) { return fs.fileSize(h); })
        .def("close", [](FileSystem& fs, WriteHandle h) { fs.close(h); })
        .def("close", [](FileSystem& fs, ReadHandle h) { fs.close(h); })
        .def("read",
             [](FileSystem& fs, ReadHandle h) {
                 auto size = fs.fileSize(h);
                 auto res = py::bytes(nullptr, size);
                 std::string_view sv = res;
                 size = fs.read(h, (char*) sv.data(), sv.size());
                 if (!size)
                     res = py::bytes();
                 return res;
             })
        .def("write", [](FileSystem& fs, WriteHandle h, std::string_view sv) { fs.write(h, sv.data(), sv.size()); })
        .def("addAttribute", [](FileSystem& fs, std::string_view& path, double val) { fs.addAttribute(path, val); })
        .def("addAttribute",
             [](FileSystem& fs, std::string_view& path, const std::string& val) { fs.addAttribute(path, val); })
        .def("addAttribute", [](FileSystem& fs, std::string_view& path, uint32_t val) { fs.addAttribute(path, val); })
        .def("addAttribute", [](FileSystem& fs, std::string_view& path, uint64_t val) { fs.addAttribute(path, val); })
        .def("getAttribute", [](FileSystem& fs, std::string_view& path) { return fs.getAttribute(path); })
        .def("remove", [](FileSystem& fs, std::string_view& path) { return fs.remove(path); })
        .def("dir",
             [](FileSystem& fs, std::string_view& path) { 
                 for (auto cur = fs.begin(path); cur; cur = fs.next(cur))
                     py::print(cur.key().m_relativePath, cur.value());
            })
        .def("commit", &FileSystem::commit)
        .def("rollback", &FileSystem::rollback)

    ;

    py::enum_<OpenMode>(m, "OpenMode")
        .value("CreateNew", OpenMode::CreateNew)
        .value("CreateAlways", OpenMode::CreateAlways)
        .value("Open", OpenMode::Open)
        .value("OpenExisting", OpenMode::OpenExisting)
        .value("ReadOnly", OpenMode::ReadOnly);

    m.def("openCompoundFile", &openCompoundFile, py::return_value_policy::take_ownership);
}
