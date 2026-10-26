// Configuração do processo de testes, feita antes do main() do Catch2.
//
// Em Debug, o MSVC mostra uma caixa de diálogo (Abort/Retry/Ignore) quando, por exemplo, um
// std::vector é indexado fora dos limites. Num teste corrido pelo ./learn essa janela ficava
// escondida e o processo parado até ao timeout. Aqui mandamos esses relatórios para o stderr e
// desligamos as janelas de erro do Windows: o teste falha (ou o processo termina) e vês a mensagem.

#if defined(_WIN32)
 #include <windows.h>
 #include <crtdbg.h>
 #include <cstdlib>

namespace
{
    const bool testEnvironmentConfigured = []
    {
        SetErrorMode (SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);

       #if defined(_DEBUG)
        _CrtSetReportMode (_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
        _CrtSetReportFile (_CRT_WARN, _CRTDBG_FILE_STDERR);
        _CrtSetReportMode (_CRT_ERROR, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
        _CrtSetReportFile (_CRT_ERROR, _CRTDBG_FILE_STDERR);
        _CrtSetReportMode (_CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
        _CrtSetReportFile (_CRT_ASSERT, _CRTDBG_FILE_STDERR);
       #endif

        _set_abort_behavior (0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
        return true;
    }();
}
#endif
