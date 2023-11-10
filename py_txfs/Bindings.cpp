
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include "CompoundFs/Composite.h"
#include "CompoundFs/PosixFile.h"
#include <CompoundFs/WindowsFile.h>
#include <CompoundFs/Path.h>
#include <CompoundFs/Overloaded.h>
#include <sstream>

using namespace TxFs;
namespace py = pybind11;

namespace
{
FileSystem openCompound(const char* path, OpenMode mode)
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


auto convertToVariant(const TreeValue& tv)
{
    using PyVariant = std::variant<uint64_t, uint32_t, double, Folder, py::tuple, py::str>;
    return tv.visit(Overloaded {
        [](const auto& val) -> PyVariant { return val; }, 
        [](FileDescriptor fd) -> PyVariant { return fd.m_fileSize; },
        [](TreeValue::Unknown u) -> PyVariant { return "Unknown"; },
        [](Version v) -> PyVariant { return py::make_tuple(v.m_major, v.m_minor, v.m_patch); } });
}

auto subFolder(FileSystem& fs, Folder folder)
{
    py::list l;
    for (auto cursor = fs.begin(Path(folder, "")); cursor; cursor = fs.next(cursor))
    {
        auto key = cursor.key();
        auto entry = py::make_tuple(key.m_parentFolder, key.m_relativePath, cursor.value());
        l.append(entry);
    }
    return l;
}


void importFiles(std::string_view source, FileSystem& fs, std::string_view destination, const std::function<void(std::string_view)>& cb)
{
    namespace sfs = std::filesystem;
    std::vector<uint8_t> buffer;
    const uint32_t BufferPages = 32;

    for (const auto& e: sfs::recursive_directory_iterator(source, sfs::directory_options::skip_permission_denied))
    {
        auto s = e.path().generic_u8string();
        auto rel = std::mismatch(source.begin(), source.end(), s.begin(), s.end()).second;
        std::string dest { destination };
        dest += std::string(rel, s.end());
        cb(dest);

        if (e.is_directory())
            fs.createPath(Path(dest));
        else if (e.is_regular_file())
        {
            buffer.resize(BufferPages * 4096ULL);
            auto wh = fs.createFile(Path(dest));
            PosixFile rf(e, OpenMode::ReadOnly);
            auto fileSizeInPages = (PageIndex) rf.fileSizeInPages();
            Interval iv(0, 0 + BufferPages);
            while (true)
            {
                iv.end() = std::min(iv.end(), fileSizeInPages);
                auto end = rf.readPages(iv, buffer.data());
                size_t size = end - buffer.data();
                fs.write(*wh, buffer.data(), size);
                if (size < buffer.size())
                    break;
                iv = Interval(iv.begin() + BufferPages, iv.begin() + 2*BufferPages);
            }
            fs.close(*wh);
        }
    }
}

} // namespace

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

    py::class_<Folder>(m, "Folder")
        .def_property_readonly_static("root", [](py::object /* self */) { return Folder::Root; })
        .def("__repr__", [](Folder folder) {
            std::ostringstream ss;
            ss << "<Folder: " << int(folder) << '>';
            return ss.str();
    });

    py::enum_<TreeValue::Type>(m, "TreeValueType")
        .value("FILE", TreeValue::Type::File)
        .value("FOLDER", TreeValue::Type::Folder)
        .value("DOUBLE", TreeValue::Type::Double)
        .value("STRING", TreeValue::Type::String)
        .value("INT", TreeValue::Type::Int64)
        .value("VERSION", TreeValue::Type::Version);


    py::class_<TreeValue>(m, "Attribute")
        .def("__str__", &strImpl)
        .def("__repr__", &reprImpl)
        .def("type_name", [](const TreeValue& tv) { return tv.getTypeName(); })
        .def("as_variant", &convertToVariant)
        .def("type", [](const TreeValue& tv) { return tv.getType(); });

    py::class_<FileSystem>(m, "FileSystem")
        .def("create_file", [](FileSystem& fs, const char* path) { return fs.createFile(path); })
        .def("append_file", [](FileSystem& fs, const char* path) { return fs.appendFile(path); })
        .def("read_file", [](FileSystem& fs, const char* path) { return fs.readFile(path); })
        .def("file_size", [](const FileSystem& fs, const char* path) { return fs.fileSize(path); })
        .def("file_size", [](const FileSystem& fs, WriteHandle h) { return fs.fileSize(h); })
        .def("file_size", [](const FileSystem& fs, ReadHandle h) { return fs.fileSize(h); })
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
        .def("add_attribute", [](FileSystem& fs, std::string_view& path, double val) { fs.addAttribute(path, val); })
        .def("add_attribute",
             [](FileSystem& fs, std::string_view& path, const std::string& val) { fs.addAttribute(path, val); })
        .def("add_attribute", [](FileSystem& fs, std::string_view& path, uint32_t val) { fs.addAttribute(path, val); })
        .def("add_attribute", [](FileSystem& fs, std::string_view& path, uint64_t val) { fs.addAttribute(path, val); })
        .def("get_attribute", [](FileSystem& fs, std::string_view& path) { return fs.getAttribute(path); })
        .def("remove", [](FileSystem& fs, std::string_view& path) { return fs.remove(path); })
        .def("dir",
             [](FileSystem& fs, std::string_view& path) {
                 for (auto cur = fs.begin(path); cur; cur = fs.next(cur))
                     py::print(cur.key().m_relativePath, cur.value());
             })
        .def("commit", &FileSystem::commit)
        .def("rollback", &FileSystem::rollback)
        .def("sub_folder", subFolder)

        ;

    py::enum_<OpenMode>(m, "OpenMode")
        .value("CREATE_NEW", OpenMode::CreateNew)
        .value("CREATE_ALWAYS", OpenMode::CreateAlways)
        .value("OPEN_ALWAYS", OpenMode::Open)
        .value("OPEN_EXISTING", OpenMode::OpenExisting)
        .value("READ_ONLY", OpenMode::ReadOnly);

    m.def("open_compound", &openCompound, py::return_value_policy::take_ownership);

    m.def("get_folder", [](const TreeValue& val) -> std::variant<Folder, py::none> {
        if (val.getType() == TreeValue::Type::Folder)
            return val.get<Folder>();
        return py::none();
    });

    m.def("import_files", &importFiles);
}
