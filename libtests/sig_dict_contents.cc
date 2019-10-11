#include <qpdf/Buffer.hh>
#include <qpdf/QPDF.hh>
#include <qpdf/QPDFObjectHandle.hh>
#include <qpdf/QPDFWriter.hh>

#include <fstream>
#include <iostream>
#include <string>
#include <cstdlib>

void usage()
{
    std::cerr
      << "Usage: sig_dict_contents [OPTION] INPUT.pdf EXPECTED.txt"
      << std::endl
      << "Option:"
      << std::endl
      << "  --object-streams=preserve|disable|generate"
      << std::endl;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
	usage();
	std::exit(2);
    }

    qpdf_object_stream_e mode = qpdf_o_preserve;
    std::string filename_input;
    std::string filename_expected;
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
	    else
	    {
		usage();
		std::exit(2);
	    }
	}
	else if (argc == i + 2)
	{
	    filename_input = argv[i];
	    filename_expected = argv[i + 1];
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
	QPDF qpdf;
	qpdf.processFile(filename_input.c_str ());

	QPDFWriter w(qpdf);
	w.setOutputMemory();
	w.setObjectStreamMode(mode);
	w.write();

	Buffer* buf = w.getBuffer();
	std::string str (reinterpret_cast<char*>(buf->getBuffer()),
			 buf->getSize());

	std::ifstream ifs(filename_expected.c_str());
	std::string expected
	  = std::string(std::istreambuf_iterator<char>(ifs),
			std::istreambuf_iterator<char>());

	if (str.find(expected) == std::string::npos)
	{
	    std::cerr << "failed" << std::endl;
	    std::exit(2);
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
