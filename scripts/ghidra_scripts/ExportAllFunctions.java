// Export every decompilable function in the program to one .c file per function.
//
// Original tooling for megahack-research. Writes pseudocode for reading and
// cross-referencing; it is NOT compilable source and is never committed to the
// repository. See docs/GOALS.md and AGENTS.md.
//
// @category HackMega

import java.io.File;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.symbol.Symbol;
import ghidra.program.model.symbol.SymbolIterator;

public class ExportAllFunctions extends GhidraScript {

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 1) {
            throw new IllegalArgumentException(
                "Usage: ExportAllFunctions.java <output-dir> [min-function-bytes]");
        }
        File outputDir = new File(args[0]);
        if (!outputDir.exists() && !outputDir.mkdirs()) {
            throw new IllegalStateException("cannot create " + outputDir);
        }
        long minBytes = args.length > 1 ? Long.parseLong(args[1]) : 0L;

        DecompInterface decompiler = new DecompInterface();
        DecompileOptions options = new DecompileOptions();
        options.setEliminateUnreachable(false);
        decompiler.setOptions(options);
        decompiler.toggleCCode(true);
        decompiler.toggleSyntaxTree(true);
        decompiler.setSimplificationStyle("decompile");
        if (!decompiler.openProgram(currentProgram)) {
            throw new IllegalStateException("Could not open program in decompiler");
        }

        FunctionIterator functions = currentProgram.getFunctionManager().getFunctions(true);
        int total = 0;
        int written = 0;
        int failed = 0;
        int skipped = 0;
        List<String> index = new ArrayList<>();

        try {
            while (functions.hasNext() && !monitor.isCancelled()) {
                if (total % 500 == 0) {
                    println("decompiled " + total + ", written " + written
                        + ", failed " + failed + ", skipped " + skipped);
                }
                Function function = functions.next();
                total++;

                if (function.getBody().getNumAddresses() < minBytes) {
                    skipped++;
                    continue;
                }
                if (function.isExternal() || function.isThunk()) {
                    skipped++;
                    continue;
                }

                Address entry = function.getEntryPoint();
                String label = safeName(function.getName());
                String fileName = String.format("%s_%s.c",
                    entry.toString().replace("::", "_"), label);
                File target = new File(outputDir, fileName);

                String body;
                try {
                    DecompileResults results =
                        decompiler.decompileFunction(function, 120, monitor);
                    if (results == null || results.decompileCompleted() == false) {
                        failed++;
                        body = "// DECOMPILE FAILED for " + function.getName() + "\n";
                    } else if (results.getDecompiledFunction() == null) {
                        failed++;
                        body = "// NO OUTPUT for " + function.getName() + "\n";
                    } else {
                        body = results.getDecompiledFunction().getC();
                        written++;
                    }
                } catch (Exception exc) {
                    failed++;
                    body = "// EXCEPTION " + exc + "\n";
                }

                try (PrintWriter out = new PrintWriter(target, StandardCharsets.UTF_8)) {
                    out.println("/*");
                    out.println(" * function : " + function.getName());
                    out.println(" * va       : " + entry);
                    out.println(" * rva      : "
                        + entry.subtract(currentProgram.getImageBase()));
                    out.println(" * signature: " + function.getSignature());
                    out.println(" * size     : " + function.getBody().getNumAddresses() + " bytes");
                    out.println(" * thunk    : " + function.isThunk());
                    out.println(" * external : " + function.isExternal());
                    out.println(" * callers  : " + function.getCallingFunctions(monitor).size());
                    out.println(" * NOTE: decompiler output. Not compilable, not committed.");
                    out.println(" */");
                    out.println(body == null ? "// (null)" : body);
                }
                index.add(entry + "\t" + function.getName() + "\t" + fileName);
                monitor.checkCancelled();
            }

            try (PrintWriter out = new PrintWriter(
                    new File(outputDir, "_index.tsv"), StandardCharsets.UTF_8)) {
                out.println("# entry\tname\tfile");
                for (String line : index) {
                    out.println(line);
                }
            }
            try (PrintWriter out = new PrintWriter(
                    new File(outputDir, "_symbols.tsv"), StandardCharsets.UTF_8)) {
                out.println("# address\tname\ttype\tsource\tnamespace");
                SymbolIterator symbols = currentProgram.getSymbolTable().getAllSymbols(true);
                while (symbols.hasNext()) {
                    Symbol symbol = symbols.next();
                    out.println(symbol.getAddress() + "\t" + symbol.getName() + "\t"
                        + symbol.getSymbolType() + "\t" + symbol.getSource() + "\t"
                        + symbol.getParentNamespace());
                }
            }
        } finally {
            decompiler.dispose();
        }

        println("DONE total=" + total + " written=" + written
            + " failed=" + failed + " skipped=" + skipped);
    }

    private static String safeName(String name) {
        StringBuilder sb = new StringBuilder(name.length());
        for (int i = 0; i < name.length() && i < 96; i++) {
            char c = name.charAt(i);
            boolean ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
                || (c >= '0' && c <= '9') || c == '_' || c == '.';
            sb.append(ok ? c : '_');
        }
        return sb.toString();
    }
}