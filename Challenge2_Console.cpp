//Timothy Edwards - "Challenge 2: KDB Files"
//Created with Microsoft Visual Studio Express 2012 for Windows Desktop

// Challenge2_Console.cpp : main project file.

#include "stdafx.h"
#include "DataProcessing.h"

using namespace System;
using namespace System::Collections;
using namespace System::Diagnostics;
using namespace System::IO;

///Initial value for Challenge 2
static int initValue = 0x4F574154;

///Run Challenge #2.
void Challenge2()
{
	//My personal path to store.kdb is this.
	String ^ fileInput = "C:\\Users\\Timothy E\\Documents\\Visual Studio for CTS\\SW_2018\\SW_2018\\" +
						  "store.kdb";

	//Asks user if default path is right.  If not, asks user to input it.
	Console::WriteLine(L"\r\nIs the following file path, to 'store.kdb', right? '" + fileInput +
					"'.\r\nIf so, presss ENTER now.  If not, enter the right path then press ENTER: ");
	
	//Has user entered his/her own path for the file?  If so, change file input as such.
	String ^ userResponse = Console::ReadLine();
	fileInput = (userResponse == "" ? fileInput : userResponse);

	//A likely exception is not finding file inputed.
	try
	{
		//Sets up file reader with file specified.
		FileStream ^ fs = gcnew FileStream(fileInput, FileMode::Open);

		//Reading HEAD.
		System::Byte MAGIC[6];
		System::Int32 ENTRY_ptr = 0;

		//Reading MAGIC
		for(int onByte = 0; onByte < 6; onByte++)
		{
			MAGIC[onByte] = fs->ReadByte();
		}

		//Reading ENTRY pointer.
		for(int onByte = 0; onByte < 4; onByte++)
		{
			ENTRY_ptr += (int)fs->ReadByte() * Math::Pow(256, onByte);
		}

		//Seek to start of ENTRY_LIST.
		fs->Seek(ENTRY_ptr, SeekOrigin::Begin);

		//Display how ENTRYs are listed.
		Console::WriteLine(L"================================================================================" + 
							"====================");
		Console::WriteLine(L"Each Entry: [NAME......{1st BLOCK's DATA}{2nd BLOCK's DATA}{3rd BLOCK's DATA}{etc...}]");
		Console::WriteLine(L"================================================================================" + 
							"====================");

		//Going through ENTRYs in ENTRY_LIST.
		for(int onEntry = 0; onEntry < 127; onEntry++)
		{
			//If this is not the first entry, add a line to ensure separation between entries.
			if(onEntry != 0)
			{
				Console::WriteLine(L"");
			}

			/*Used to check if first four bytes (of ENTRY) are each 0xFF.  If they are, program has
			already read past all of the ENTRYs in ENTRY_LIST*/
			int firstFourBytesAdded = 0;

			//Go through ENTRY NAME characters and write them.
			Console::Write("[");
			for(int onChar = 0; onChar < 16; onChar++)
			{
				//Attempt to read a NAME character.
				Char theChar = (Char)fs->ReadByte();

				/*If NULL char encountered, end of NAME is reached.  Seek until space allocated for ENTRY NAME
				is reached.*/
				if(theChar == '\0')
				{
					fs->Seek(15 - onChar, SeekOrigin::Current);
					break;
				}

				//Writes NAME character to console.
				Console::Write(theChar);

				/*Loop used to help check if first four bytes of entry are all 0xFF, in which case end of
				ENTRY_LIST has been reached.*/
				if(onChar < 4)
				{
					firstFourBytesAdded += (int)theChar;
				}
			}

			//Writes separator between ENTRY NAME and ENTRY contents.
			Console::Write("......");

			/*If first four bytes added together are 1020, each byte is 0xFF.  Four of such bytes means
			end of ENTRY_LIST.*/
			if(firstFourBytesAdded == 1020)
			{
				//Preivously outputted end of ENTRY list bytes as an ENTRY name.  Starting to undo such output.
				Console::CursorLeft -= 11;

				//At this point, no more ENTRYs are left.
				Console::Write(L"[No more entries left]\r\n");

				//Breaking out of loop that iterates through ENTRYs.
				break;
			}

			//Go through BLOCK_LIST in ENTRY.  Start by reading pointer to BLOCK_LIST.
			int BLOCK_LIST_ptr = 0;
			for(int onByte = 0; onByte < 4; onByte++)
			{
				BLOCK_LIST_ptr += (int)fs->ReadByte() * Math::Pow(256, onByte);
			}

			//Seeking to start of BLOCK_LIST.
			fs->Seek(BLOCK_LIST_ptr, SeekOrigin::Begin);

			//Going through BLOCKs in BLOCK_LIST.
			while(1)
			{
				//Going through BLOCK SIZE characters.
				int SIZE = (int)fs->ReadByte() + (int)fs->ReadByte() * 256;

				//Going through BLOCK DATA characters.  First, getting data pointer
				array<Byte> ^ Data_ptr_bytes = gcnew array<Byte>(5);
				fs->Read(Data_ptr_bytes, 0, 4);
				int DATA_ptr = (int)Data_ptr_bytes[0] + (int)Data_ptr_bytes[1] * 256 +
							(int)Data_ptr_bytes[2] * Math::Pow(256, 2) + (int)Data_ptr_bytes[3] *
							Math::Pow(256, 3);

				/*If the first four bytes in the BLOCK are 0xFF, then the end of the BLOCK_LIST is
				reached and a BLOCK is not being dealt with.*/
				if((SIZE == 0xFFFF) && ((int)Data_ptr_bytes[0] == 0xFF) && ((int)Data_ptr_bytes[1]
				== 0xFF))
				{
					//Set position to next ENTRY.
					ENTRY_ptr += 20;
					fs->Position = ENTRY_ptr;

					break;
				}

				//Seeking to data pointer.
				fs->Seek(DATA_ptr, SeekOrigin::Begin);

				//To hold a DATA chunk.
				unsigned char * DATA = new unsigned char[SIZE];

				//Going through DATA at DATA pointer's target.
				for(int onByte = 0; onByte < SIZE; onByte++)
				{
					//Storing character that has not yet been crypted.
					Char currentChar = (Char)fs->ReadByte();
					DATA[onByte] = currentChar;
				}

				//Crypting data and then outputting it.
				unsigned char * DATA_crypted =  Crypt(DATA, SIZE, initValue);

				//Before outputting BLOCK's DATA, output a starting brace: {
				Console::Write("{");

				//Output crypted data character by character.
				for(int onChar = 0; onChar < SIZE; onChar++)
				{
					Char currentChar = (Char)DATA_crypted[onChar];
					Console::Write(currentChar);
				}

				//Once a BLOCK's DATA is done being outputted, output closing brace: '}'
				Console::Write("}");

				//Attempts to seek to next BLOCK in BLOCK_LIST.
				fs->Position = BLOCK_LIST_ptr + 6;
			}

			//Indicates ENTRY is over.
			Console::Write("]\r\n");
		}

	}
	catch(System::Exception ^ex)
	{
		//Explains exception, which likely came from invalid file path inputed.
		Console::WriteLine(L"ERROR: " + ex->Message);
	}

	//Indicates ENTRYs are done being read and tells user how to close program.
	Console::WriteLine(L"");
	Console::WriteLine(L"====================================================================================");
	Console::WriteLine(L"");
	Console::WriteLine(L"All ENTRYs found.  Press ENTER to exit.");
	if(Console::ReadKey().Key == ConsoleKey::Enter)
	{
		Environment::Exit(0);
	}
}

int main(array<System::String ^> ^args)
{
	//Introduce Challenge 2 Program.  Ask user to proceed with running challenge.
	Console::WriteLine(L"*********************************************************************");
	Console::WriteLine(L"Challenge 2: KDB Files");
	Console::WriteLine(L"*********************************************************************");
	Console::WriteLine(L"Press ENTER to run challenge...");
	if(Console::ReadKey().Key == ConsoleKey::Enter)
	{
		Challenge2();
	}
	
    return 0;
}
