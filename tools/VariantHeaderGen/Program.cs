using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text;

namespace VariantHeaderGen
{
    class Program
    {
        static string NewLine = "\r\n";
        static string VariantFormat = "\t{2}(opcode, {0}, {1}, content)";
        static string VariantArgs = "(opcode, content)";

        static UInt16[] AllAllowedFlagsExpressions =
        {
            0xBF,
            0x40,
            0x3FF,
            0x15,
            0x83F,
            0xC3F,
            0x23F,
            0x35,
            0xABF,
            0x555,
            0x55,
            0x33F,
            0x3F,
            0x415,
            0x115
        };

        static HashSet<UInt16> AllFlags = new HashSet<ushort>(); 
        static void Main(string[] args)
        {
            string outputHeader = new string("#pragma once" + NewLine);
            outputHeader += "// Macros for all the possibe combinations of flags with what is allowed, enjoy." + NewLine;
            outputHeader += "// Sources from available programs." + NewLine;

            // Macros for all the possibe combinations of flags with what is allowed, enjoy.
            // Sources from available programs.

            var r86Files = Directory.GetFiles(".", "*.r86");

            foreach (string file in r86Files)
            {
                using (BinaryReader reader = new BinaryReader(File.Open(file, FileMode.Open)))
                {
                    UInt32 magicIdent = reader.ReadUInt32();
                    UInt32 heapSize = reader.ReadUInt32();

                    if (heapSize > 0)
                    {
                        reader.BaseStream.Seek(heapSize, SeekOrigin.Current);
                    }

                    UInt32 instructions = reader.ReadUInt32();

                    for (UInt32 i = 0; i < instructions; i++)
                    {
                        byte opcode = reader.ReadByte();
                        UInt16 flags = reader.ReadUInt16();
                        reader.BaseStream.Seek(sizeof(UInt64), SeekOrigin.Current);

                        AllFlags.Add(flags);
                    }
                }
            }

            foreach (UInt16 allowedFlags in AllAllowedFlagsExpressions)
            {
                outputHeader += NewLine + "// Variant " + allowedFlags.ToString() + NewLine;
                outputHeader += "#define VARIANT_" + allowedFlags.ToString("X") + VariantArgs;

                foreach (UInt16 flag in AllFlags)
                {
                    if ((flag <= allowedFlags && (flag & allowedFlags) > 0))
                    {
                        outputHeader += " \\" + NewLine + string.Format(VariantFormat, flag, allowedFlags, "VARIANT");
                    }
                }

                outputHeader += " \\" + NewLine + string.Format(VariantFormat, "m_ProcessorState->ps_ProgramLineFlags", allowedFlags, "FALLBACK");
            }

            using (FileStream fs = File.Open("../src/Raptor86/Variants.h", FileMode.Truncate))
            {
                UTF8Encoding uniEncoding = new UTF8Encoding();
                fs.Write(uniEncoding.GetBytes(outputHeader), 0, uniEncoding.GetByteCount(outputHeader));
            }
        }
    }
}
