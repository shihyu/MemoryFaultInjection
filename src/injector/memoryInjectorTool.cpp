/*
*  Author: HIT CS HDMC team.
*  Create: 2010-3-13 8:50:12
*  Last modified: 2010-6-16 14:08:59
*  Description:
*  	Memory fault injection tool.
*/
#include <sys/wait.h>
#include <signal.h>
#include "memoryEngine.h"
#include "memoryInjectorTool.h"
#include "memoryInjectorTool.h"


InjectorTool::InjectorTool()
:   m_injector(NULL),             // the memory fault injector object
    m_argc(0),                  // the count of the arguement
    m_argv(NULL),                // the arguement list
    m_hasFaultTable(false),
    m_memoryFault(text_area, -1, word_0, 0, 0) //  默认的信息
{
    this->m_targetPid = -1;
	this->m_exeArguments = NULL;
	this->m_memoryFaultTablePath.clear();
}

InjectorTool::~InjectorTool()
{
    delete this->m_injector;
	this->m_memoryFaultTablePath.clear();
}


// create the InjectorTool object use the system arg...
Injector * InjectorTool::CreateInjector( int argc, char **argv )
{
    this->m_argc = argc;
    this->m_argv = argv;

    MemoryFault faultTmp(text_area, -1, word_0, 1, 3);  //  默认的信息

#ifdef DEBUG
    printf("this->m_argc = %d\n", this->m_argc);
    for(int i = 0; i < this->m_argc; i++)
    {
        printf("this->m_argv[%d] = %s\n", i, this->m_argv[i]);
    }
#endif

    dprintf("now m_argc = %d, comm = %s\n", m_argc, m_argv[0]);
	this->m_argc--;
	this->m_argv++;

    /// get arguments
	while(this->m_argc > 0)
	{
        dprintf("now m_argc = %d, m_argv[0] = %s, m_argv[1] = %s\n", m_argc, m_argv[0], m_argv[1]);

        if(strcmp(this->m_argv[0], "-e") == 0)
		{
			this->m_exeArguments = this->m_argv + 1;
#ifdef BUGS
            dcout <<endl <<"BUG002--[" <<__FILE__  <<", " <<__func__ <<", "<<__LINE__ <<"]--exe = " <<this->m_exeArguments[0] <<", address = " <<this->m_exeArguments <<endl;
#endif

            dcout <<"The process you want to inject : length = " <<argc - 1 <<", comm = \"";
            for(int i = 0; i < m_argc - 1; i++)
            {
                i && dcout <<" ";
                dcout <<this->m_exeArguments[i];
            }
            dcout <<"\"" <<endl;
            //this->m_exeArguments[m_argc] = NULL;
            break;
		}
		else if(strcmp(this->m_argv[0], "-p") == 0)
		{
			this->m_targetPid = atoi(this->m_argv[1]);

            dcout <<"The pid of the process you want to inject is " <<this->m_argv[1] <<" == " <<this->m_targetPid <<endl;

			break;
		}
        else if(strcmp(this->m_argv[0], "-c") == 0)          //  -c to set the config file...
		{
            //  you can see we read the config file in
            //  int InjectorTool::initFaultTable( void )
            //
            //  add by gatieme @
            //  or you can use the
            //  -l --location   stack|data|text
            //  -m --mode       random | address
            //  -t --type
            //      --time      1
            //      --timeout   3
			this->m_memoryFaultTablePath = this->m_argv[1];

            this->m_hasFaultTable = true;

		}
        else if( strcmp(this->m_argv[0], "-l") == 0 )
        {
		    if( faultTmp.SetLocation(this->m_argv[1]) == true )
		    {
		        dcout << "read the Location : " <<this->m_argv[1] << endl;
		        //faultTmp.m_location = stack_area;
		    }
		    else
		    {
		        cerr << "Error, undefined fault location : " <<this->m_argv[1] << endl;
			    return NULL;
		    }
        }
        else if ( strcmp(this->m_argv[0], "-m") == 0 )
        {
		    if( faultTmp.SetMode(this->m_argv[1]) == true )
		    {
		        dcout << "read the Mode : " <<this->m_argv[1] << endl;
		    }
		    else
		    {
		        cerr << "Error, undefined fault mode : " <<this->m_argv[1] << endl;
			    return NULL;
		    }
        }
        else if ( strcmp(this->m_argv[0], "-t") == 0 )
        {
		    if( faultTmp.SetFaultType(this->m_argv[1]) == true )
		    {
		        dcout << "read the Type : " <<this->m_argv[1] << endl;
            }
		    else
		    {
		        cerr << "Error, undefined fault mode : " <<this->m_argv[1] << endl;
            }
        }
        else if( strcmp(this->m_argv[0], "--time") == 0)
        {
		    if( faultTmp.SetTime(this->m_argv[1]) == true )
		    {
		        dcout << "read the Time : " <<this->m_argv[1] << endl;
            }
		    else
		    {
		        cerr << "Error, undefined fault time : " <<this->m_argv[1] << endl;
            }
        }
        else if( strcmp(this->m_argv[0], "--timeout") == 0)
        {
		    if( faultTmp.SetTimeout(this->m_argv[1]) == true )
		    {
		        dcout << "read the Timeout : " <<this->m_argv[1] << endl;
            }
		    else
		    {
		        cerr << "Error, undefined fault timeout : " <<this->m_argv[1] << endl;
            }
        }
        this->m_argc -=2 ;
        this->m_argv += 2;
    }
    dcout <<__LINE__ <<endl;
    ///return 0;

    //  如果使用了-c参数指定了故障注入表
    if( this->m_hasFaultTable == true )     //  读取故障植入表的信息
    {
        if( this->initFaultTable() == RT_FAIL )    //  故障注入表会存入emoryFaultTable
        {
            cerr <<"Eror, init the faule table faild..." <<endl;
            return NULL;
        }
    }
    else
    {
        this->m_memoryFault = faultTmp;
        this->m_memoryFaultTable.push_back(faultTmp);   //  同时将设置的配置参数存入memoryFaultTable
    }

    //  依据配置参数创建注入工具
    Injector * pInjector = new Injector(
            this->m_targetPid,                          //  故障注入的进程号
            this->m_exeArguments,                       //  故障注入的程序名
            this->m_memoryFaultTable);                  //  故障注入的注入表
    if ( pInjector == NULL)
	{
		return NULL;
	}

    this->m_injector = pInjector;

    return this->m_injector;
}


int InjectorTool::initFaultTable( void )
{
    //  eg text random word_0 1 3
    //  [0] text | data | stack
    //  [1] random
    //  [2] word_0 |
    //  [3] 1
    //  [4] 3

    string line;

    if( this->m_memoryFaultTablePath.empty() )
	{
		cerr << "Error:no existing fault table" << endl;

        return RT_FAIL;
	}

	ifstream infile;
	infile.open( this->m_memoryFaultTablePath.c_str(), ios::in );
	if( !infile )
	{
		cerr << "Error:unable to open file:" << this->m_memoryFaultTablePath << endl;

        return RT_FAIL;
	}
#ifdef DEBUG
    else
    {
        std::cout <<"open the config file \"" <<this->m_memoryFaultTablePath <<"\" success..." <<std::endl;
    }
#endif // DEBUG

    std::string strLine;
    std::string strTmp;
	MemoryFault  faultTmp;

	while( getline(infile, strLine, '\n') )
	{
#ifdef DEBUG
        std::cout <<"read \"" <<strLine <<"\"" <<std::endl;
#endif // DEBUG

		//bind istream to the strLine
		istringstream stream(strLine);

		/// location: text or data
		strTmp.clear();
		stream >> strTmp;
		if( strTmp.empty() )
		{
			cerr << "Error:fault table format errno" << endl;
			return RT_FAIL;
		}
		else if( strTmp == "text" || strTmp == "TEXT" )
		{
			faultTmp.m_location = text_area;
		}
		else if( strTmp == "data" || strTmp == "DATA" )
		{
			faultTmp.m_location = data_area;
		}
		else if( strTmp == "stack" || strTmp == "STACK" )
		{
			faultTmp.m_location = stack_area;
		}
		else
		{
			cerr << "Error:undefined fault location" << endl;
			return RT_FAIL;
		}

		/// memory addr or random
		strTmp.clear();
		stream >> strTmp;
		if( strTmp.empty() )
		{
			cerr << "Error:fault table format errno" << endl;
			return RT_FAIL;
		}

		if( strTmp == "random" || strTmp == "RANDOM" )
		{
			faultTmp.m_addr = -1;
		}
		else
		{
			int iRet = sscanf( strTmp.c_str(), "%lx", &faultTmp.m_addr );

			if( iRet != 1 )
            {
                return RT_FAIL;
            }
		}

		/// fault type: one_bit_flip, etc
		strTmp.clear();
		stream >> strTmp;
		if( strTmp.empty() )
		{
			cerr << "Error:fault table format errno" << endl;

            return RT_FAIL;
		}
		else if( strTmp == "one_bit_0" )
		{
			faultTmp.m_faultType = one_bit_0;
		}
		else if( strTmp == "one_bit_1" )
		{
			faultTmp.m_faultType = one_bit_1;
		}
		else if( strTmp == "one_bit_flip" )
		{
			faultTmp.m_faultType = one_bit_flip;
		}

		else if( strTmp == "word_0" )
		{
			faultTmp.m_faultType = word_0;
		}
		else if( strTmp == "page_0" )
		{
			faultTmp.m_faultType = page_0;
		}
/*
		else if( strTmp == "two_bit_0" )
		{
			faultTmp.faultType = two_bit_0;
		}
		else if( strTmp == "two_bit_1" )
		{
			faultTmp.faultType = two_bit_1;
		}
		else if( strTmp == "two_bit_flip" )
		{
			faultTmp.faultType = two_bit_flip;
		}
		else if( strTmp == "low_8_0" )
		{
			faultTmp.faultType = low_8_0;
		}
		else if( strTmp == "low_8_1" )
		{
			faultTmp.faultType = low_8_1;
		}
		else if( strTmp == "low_8_error" )
		{
			faultTmp.faultType = low_8_error;
		}
		else if( strTmp == "word_0" )
		{
			faultTmp.faultType = word_0;
		}
		else if( strTmp == "word_1" )
		{
			faultTmp.faultType = word_1;
		}
		else if( strTmp == "word_error" )
		{
			faultTmp.faultType = word_error;
		}
		else if( strTmp == "page_0" )
		{
			faultTmp.faultType = page_0;
		}
		else if( strTmp == "page_1" )
		{
			faultTmp.faultType = page_1;
		}
		else if( strTmp == "page_error" )
		{
			faultTmp.faultType = page_error;
		}
*/
		else
		{
			cerr << "Error:undefined fault type" << endl;
			return RT_FAIL;
		}

		/// how long fault exits, not support yet
		strTmp.clear();
		stream >> strTmp;

        if( strTmp.empty() )
		{
			cerr << "Error:fault table format errno" << endl;
			return RT_FAIL;
		}
		faultTmp.m_time = atoi( strTmp.c_str() );

		/// timeout
		strTmp.clear();
		stream >> strTmp;
		if( strTmp.empty() )
		{
			cerr << "Error:fault table format errno" << endl;
			return RT_FAIL;
		}
		faultTmp.m_timeout = atoi( strTmp.c_str() );

		//  add a fault into fault vector
		this->m_memoryFaultTable.push_back( faultTmp );
	}

	infile.close();
	return RT_OK;
}

int InjectorTool::startInjection( void )
{
    return this->m_injector->startInjection( );
}



void InjectorTool::usage()
{
    Injector::usage( );
}
