#include <qpdf/Buffer.hh>
#include <qpdf/PointerHolder.hh>
#include <qpdf/QPDF.hh>
#include <qpdf/QPDFObject.hh>
#include <qpdf/QPDFObjectHandle.hh>
#include <qpdf/QPDFObjGen.hh>
#include <qpdf/QPDFWriter.hh>

#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <vector>
#include <cstdlib>

void usage()
{
    std::cerr
      << "Usage: get_renumber [OPTION] INPUT.pdf"
      << std::endl
      << "Option:"
      << std::endl
      << "  --object-streams=preserve|disable|generate"
      << std::endl
      << "  --linearize"
      << std::endl
      << "  --preserve-unreferenced"
      << std::endl;
}

bool compare(QPDFObjectHandle a, QPDFObjectHandle b)
{
    static std::set<QPDFObjGen> visited;
    if (a.isIndirect())
    {
	if (visited.count(a.getObjGen()))
	{
	    return true;
	}
	visited.insert(a.getObjGen());
    }

    if (a.getTypeCode() != b.getTypeCode())
    {
	return false;
    }

    switch (a.getTypeCode())
    {
    case QPDFObject::ot_boolean:
	if (a.getBoolValue() != b.getBoolValue())
	{
	    return false;
	}
	break;
    case QPDFObject::ot_integer:
	if (a.getIntValue() != b.getIntValue())
	{
	    return false;
	}
	break;
    case QPDFObject::ot_real:
	if (a.getRealValue() != b.getRealValue())
	{
	    return false;
	}
	break;
    case QPDFObject::ot_string:
	if (a.getStringValue() != b.getStringValue())
	{
	    return false;
	}
	break;
    case QPDFObject::ot_name:
	if (a.getName() != b.getName())
	{
	    return false;
	}
	break;
    case QPDFObject::ot_array:
	{
	    std::vector<QPDFObjectHandle> objs_a = a.getArrayAsVector();
	    std::vector<QPDFObjectHandle> objs_b = b.getArrayAsVector();
	    size_t items = objs_a.size();
	    if (items != objs_b.size())
	    {
		return false;
	    }

	    for (size_t i = 0; i < items; ++i)
	    {
		if (!compare(objs_a[i], objs_b[i]))
		{
		    return false;
		}
	    }
	}
	break;
    case QPDFObject::ot_dictionary:
	{
	    std::set<std::string> keys_a = a.getKeys();
	    std::set<std::string> keys_b = b.getKeys();
	    if (keys_a != keys_b)
	    {
		return false;
	    }

	    for(std::set<std::string>::iterator iter = keys_a.begin();
		iter != keys_a.end(); ++iter)
	    {
		if (!compare(a.getKey(*iter), b.getKey(*iter)))
		{
		    return false;
		}
	    }
	}
	break;
    case QPDFObject::ot_null:
	break;
    case QPDFObject::ot_stream:
	std::cout << "stream objects are not compared" << std::endl;
	break;
    default:
	std::cerr << "object type error" << std::endl;
	std::exit(2);
    }

    return true;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
	usage();
	std::exit(2);
    }

    qpdf_object_stream_e mode = qpdf_o_preserve;
    bool blinearize = false;
    bool bpreserve_unreferenced = false;
    std::string filename_input;
    for (int i = 1; i < argc; ++i)
    {
	if (argv[i][0] == '-' && argv[i][1] == '-')
	{
	    std::string opt = argv[i];
	    if (opt == "--object-streams=preserve")
	    {
		mode = qpdf_o_preserve;
	    }
	    else if (opt == "--object-streams=disable")
	    {
		mode = qpdf_o_disable;
	    }
	    else if (opt == "--object-streams=generate")
	    {
		mode = qpdf_o_generate;
	    }
	    else if (opt == "--linearize")
	    {
		blinearize = true;
	    }
	    else if (opt == "--preserve-unreferenced")
	    {
		bpreserve_unreferenced = true;
	    }
	    else
	    {
		usage();
		std::exit(2);
	    }
	}
	else if (argc == i + 1)
	{
	    filename_input = argv[i];
	    break;
	}
	else
	{
	    usage();
	    std::exit(2);
	}
    }

    try
    {
	QPDF qpdf_in;
	qpdf_in.processFile(filename_input.c_str ());
	std::vector<QPDFObjectHandle> objs_in = qpdf_in.getAllObjects();

	QPDFWriter w(qpdf_in);
	w.setOutputMemory();
	w.setObjectStreamMode(mode);
	w.setLinearization(blinearize);
	w.setPreserveUnreferencedObjects(bpreserve_unreferenced);
	w.write();

	PointerHolder<Buffer> buf = w.getBuffer();

	QPDF qpdf_ren;
	qpdf_ren.processMemoryFile("renumbered",
				   reinterpret_cast<char*>(buf->getBuffer()),
				   buf->getSize());

	for (std::vector<QPDFObjectHandle>::iterator iter = objs_in.begin();
	     iter != objs_in.end(); ++iter)
	{
	    QPDFObjGen og_in = iter->getObjGen();
	    QPDFObjGen og_ren = w.getRenumberedObjGen(og_in);

	    std::cout
	      << "input "
	      << og_in.getObj() << "/" << og_in.getGen()
	      << " -> renumbered "
	      << og_ren.getObj() << "/" << og_ren.getGen()
	      << std::endl;

	    if (og_ren.getObj() == 0)
	    {
		std::cout << "deleted" << std::endl;
		continue;
	    }

	    if (!compare(*iter, qpdf_ren.getObjectByObjGen(og_ren)))
	    {
		std::cerr
		  << "different"
		  << std::endl;
		std::exit(2);
	    }
	}

	std::cout << "succeeded" << std::endl;
    }
    catch (std::exception& e)
    {
	std::cerr << e.what() << std::endl;
	std::exit(2);
    }

    return 0;
}
