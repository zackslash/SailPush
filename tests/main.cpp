#include <QtTest>
#include "tst_message.cpp"
#include "tst_messagestore.cpp"
#include "tst_sailpushclient.cpp"
#include "tst_websocketmanager.cpp"
#include "tst_credentialstore.cpp"
#include "tst_networkmonitor.cpp"
#include "tst_cpukeepalive.cpp"

int main(int argc, char *argv[])
{
    int status = 0;

    {
        TestMessage tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestMessageStore tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestSailPushClient tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestWebSocketManager tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestCredentialStore tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestNetworkMonitor tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestCpuKeepalive tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    return status;
}
