#include <qpdf/QPDF.hh>
#include <qpdf/QPDFObjectHandle.hh>

#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <cstdlib>

void usage()
{
    std::cerr
      << "Usage: test_parsedoffset INPUT.pdf"
      << std::endl;
}

bool less_pair(const std::pair<qpdf_offset_t, QPDFObjectHandle> &lhs,
	       const std::pair<qpdf_offset_t, QPDFObjectHandle> &rhs)
{
    if (lhs.first < rhs.first)
    {
	return true;
    }
    return false;
}

void walk(size_t stream_number, QPDFObjectHandle obj,
	  std::vector<std::vector<std::pair<qpdf_offset_t, QPDFObjectHandle>>>
	  &result)
{
    std::pair<qpdf_offset_t, QPDFObjectHandle> p =
      std::make_pair(obj.getParsedOffset(), obj);
    if (result.size() < stream_number + 1)
    {
	result.resize(stream_number + 1);
    }
    result[stream_number].push_back(p);

    if (obj.isArray())
    {
	std::vector<QPDFObjectHandle> array = obj.getArrayAsVector();
	for(std::vector<QPDFObjectHandle>::iterator iter = array.begin();
	    iter != array.end(); ++iter)
	{
	    if (iter->isIndirect())
	    {
		walk(stream_number, *iter, result);
	    }
	}
    }
    else if(obj.isDictionary())
    {
	std::set<std::string> keys = obj.getKeys();
	for(std::set<std::string>::iterator iter = keys.begin();
	    iter != keys.end(); ++iter)
	{
	    QPDFObjectHandle item = obj.getKey(*iter);
	    if(!item.isIndirect())
	    {
		walk(stream_number, item, result);
	    }
	}
    }
    else if(obj.isStream())
    {
	walk(stream_number, obj.getDict(), result);
    }
}

std::vector<std::vector<std::pair<qpdf_offset_t, QPDFObjectHandle>>>
process(std::string fn)
{
    std::vector<std::vector<std::pair<qpdf_offset_t, QPDFObjectHandle>>>
      result;

    QPDF qpdf;
    qpdf.processFile(fn.c_str());
    std::vector<QPDFObjectHandle> objs = qpdf.getAllObjects();
    std::map<QPDFObjGen, QPDFXRefEntry> xrefs = qpdf.getXRefTable();

    for (std::vector<QPDFObjectHandle>::iterator iter = objs.begin();
	 iter != objs.end(); ++iter)
    {
	if (xrefs.count(iter->getObjGen()) == 0)
	{
	    std::cerr
	      << iter->getObjectID()
	      << "/"
	      << iter->getGeneration()
	      << " is not found in xref table"
	      << std::endl;
	    std::exit(2);
	}

	QPDFXRefEntry xref = xrefs[iter->getObjGen()];
	size_t stream_number;

	switch (xref.getType())
	{
	case 0:
	    std::cerr
	      << iter->getObjectID()
	      << "/"
	      << iter->getGeneration()
	      << " xref entry is free"
	      << std::endl;
	    std::exit(2);
	case 1:
	    stream_number = 0;
	    break;
	case 2:
	    stream_number = static_cast<size_t>(xref.getObjStreamNumber());
	    break;
	default:
	    std::cerr << "unknown xref entry type" << std::endl;
	    std::exit(2);
	}

	walk(stream_number, *iter, result);
    }

    return result;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
	usage();
	std::exit(2);
    }

    try
    {
	std::vector<std::vector<std::pair<qpdf_offset_t, QPDFObjectHandle>>>
	  table = process(argv[1]);

	for (size_t i = 0; i < table.size(); ++i)
	{
	    if (table[i].size() == 0)
	    {
		continue;
	    }

	    std::sort(table[i].begin(), table[i].end(), less_pair);
	    if (i == 0)
	    {
		std::cout << "--- objects not in streams ---" << std::endl;
	    }
	    else
	    {
		std::cout
		  << "--- objects in stream " << i << " ---" << std::endl;
	    }

	    for (std::vector<
		   std::pair<qpdf_offset_t, QPDFObjectHandle>>::iterator
		   iter = table[i].begin();
		 iter != table[i].end(); ++iter)
	    {
		std::cout
		  << "offset = "
		  << iter->first
		  << " (0x"
		  << std::hex << iter->first << std::dec
		  << "), ";

		if (iter->second.isIndirect())
		{
		    std::cout
		      << "indirect "
		      << iter->second.getObjectID()
		      << "/"
		      << iter->second.getGeneration()
		      << ", ";
		}
		else
		{
		    std::cout << "direct, ";
		}

		std::cout
		  << iter->second.getTypeName()
		  << std::endl;
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
