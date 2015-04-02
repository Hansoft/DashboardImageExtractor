

#include "../../HPMSdkCpp.h"

#ifdef _MSC_VER
#include <tchar.h>
#include <conio.h>
#include <windows.h>
#else
#include <sys/time.h>
#include <sys/select.h>
#include <termios.h>
#if defined(__GNUC__)
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>
#endif
#endif
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <set>

using namespace std;
using namespace HPMSdk;

#ifndef _MSC_VER
using HPMifstream = ifstream;
int _kbhit()
{
    static const int STDIN = 0;
    static bool bInitialized = false;
    
    if (!bInitialized)
    {
        termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        bInitialized = true;
    }
    
    timeval timeout;
    fd_set rdset;
    
    FD_ZERO(&rdset);
    FD_SET(STDIN, &rdset);
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    
    return select(STDIN + 1, &rdset, NULL, NULL, &timeout);
}
#else
using HPMifstream = wifstream;

#endif

#ifdef __GNUC__


struct CProcessSemaphore
{
    pthread_cond_t m_Cond;
    pthread_mutex_t m_Mutex;
    int m_Counter;
};

#endif

class CHansoftSDKSample_Simple : public HPMSdkCallbacks
{
public:
    
    bool ReadConfig(const HPMString& ConfigPath)
    {
        HPMifstream ReadFile(ConfigPath);
        
        if (!ReadFile.good()) {
            cout << "Unable to open config file. Check file path." << endl;
            ReadFile.close();
            return false;
        }
        
        HPMString MapKey;
        HPMString MapInput;
        while (!ReadFile.eof()) {
            
            getline(ReadFile, MapKey, NInternal_C::HPMCharType('='));
            getline(ReadFile, MapInput);
            
            //HPMString HPMInput = hpm_str(MapInput);
            
            m_ConfigMap[MapKey] = MapInput;
            
        }
        
        return true;
    }
    
    
    bool LoadCharts(vector<HPMUniqueID> &Id, const HPMString &FilePath)
    {
        ifstream ReadFile(FilePath);
        if (!ReadFile.good())
        {
            cout << "Unable to open chart ID file. Check file path." << endl;
            ReadFile.close();
            return false;
        }
        
        string Line;
        while (!ReadFile.eof())
        {
            getline(ReadFile, Line);
            try
            {
                int Convert = stoi(Line);
                Id.push_back(Convert);
                
            }
            catch (const exception& e)
            {
                cout << "Unable to convert line. '" << Line << "'" << e.what() << endl;
            }
        }
        
        if (!ReadFile.eof()){
            cout << "Error reading chart ID file.\n";
            ReadFile.close();
            return false;
        }
        
        ReadFile.close();
        return true;
    }
    
    bool IsConfigValid() const
    {
        static const vector<HPMString> ValidationVector = { hpm_str("HansoftServerAddress"), hpm_str("HansoftServerPort"), hpm_str("Database"), hpm_str("SDKUserName"), hpm_str("SDKUserPassword"), hpm_str("ChartIDFilePath"), hpm_str("OutputFolderPath"), hpm_str("ResultSetToolPath"), hpm_str("OutputResolution"), hpm_str("OutputFormat"), hpm_str("RefreshRate") };
        
        for (size_t i = 0; i < ValidationVector.size(); ++i)
        {
            if (m_ConfigMap.find(ValidationVector[i]) == m_ConfigMap.end())
            {
                wstring Key(ValidationVector[i].begin(), ValidationVector[i].end());
                
                wcout << "The following config setting is missing: " << Key << endl << "Check config file." << endl;
                return false;
            }
        }
        
        return true;
    }
    
    
    HPMSdkSession *m_pSession;
    bool m_bBrokenConnection;
    HPMString m_ConfigPath;
    
    virtual void On_ProcessError(EHPMError _Error)
    {
        HPMString SdkError = HPMSdkSession::ErrorToStr(_Error);
        wstring Error(SdkError.begin(), SdkError.end());
        
        wcout << "On_ProcessError: " << Error << "\r\n";
        m_bBrokenConnection = true;
    }
    
    HPMUInt64 m_NextUpdate;
    HPMUInt64 m_NextConnectionAttempt;
#ifdef _MSC_VER
    HANDLE m_ProcessSemaphore;
#elif __GNUC__
    CProcessSemaphore m_ProcessSemaphore;
#endif
    HPMNeedSessionProcessCallbackInfo m_ProcessCallbackInfo;
    
    CHansoftSDKSample_Simple(HPMString ConfigFilePath)
    {
        m_ConfigPath = ConfigFilePath;
        m_pSession = NULL;
        m_NextUpdate = 0;
        m_NextConnectionAttempt = 0;
        m_bBrokenConnection = false;
        // Create the event that will be signaled when the SDK needs processing.
        #ifdef _MSC_VER
        m_ProcessSemaphore = CreateSemaphore(NULL, 0, 1, NULL); // Behaves like an auto reset event.
        #elif __GNUC__
        pthread_cond_init(&m_ProcessSemaphore.m_Cond, NULL);
        pthread_mutex_init(&m_ProcessSemaphore.m_Mutex, NULL);
        m_ProcessSemaphore.m_Counter = 1;
#endif
    }
    
    ~CHansoftSDKSample_Simple()
    {
#ifdef _MSC_VER
        if (m_ProcessSemaphore)
            CloseHandle(m_ProcessSemaphore);
#elif __GNUC__
        pthread_mutex_destroy(&m_ProcessSemaphore.m_Mutex);
        pthread_cond_destroy(&m_ProcessSemaphore.m_Cond);
#endif
        
        if (m_pSession)
        {
            DestroyConnection();
        }
    }
    
#ifdef __GNUC__
    bool SemWait(int _Timeout)
    {
        time_t Now;
        time(&Now);
        
        timespec Ts;
        Ts.tv_sec = Now;
        Ts.tv_nsec = _Timeout * 1000000;
        
        pthread_mutex_lock(&m_ProcessSemaphore.m_Mutex);
        while (m_ProcessSemaphore.m_Counter == 0)
        {
            if (pthread_cond_timedwait(&m_ProcessSemaphore.m_Cond, &m_ProcessSemaphore.m_Mutex, &Ts) == ETIMEDOUT)
            {
                pthread_mutex_unlock(&m_ProcessSemaphore.m_Mutex);
                return true;
            }
        }
        --m_ProcessSemaphore.m_Counter;
        pthread_mutex_unlock(&m_ProcessSemaphore.m_Mutex);
        return true;
    }
#endif
    
    HPMUInt64 GetTimeSince1970()
    {
#ifdef _MSC_VER
        FILETIME Time;
        GetSystemTimeAsFileTime(&Time);
        
        return (HPMUInt64)((((ULARGE_INTEGER &)Time).QuadPart / 10) - 11644473600000000);
#else
        timeval Time;
        gettimeofday(&Time, NULL);
        return (HPMUInt64)Time.tv_sec * 1000000;
#endif
    }
    
    static void NeedSessionProcessCallback(void *_pSemaphore)
    {
#ifdef _MSC_VER
        ReleaseSemaphore(_pSemaphore, 1, NULL);
#elif __GNUC__
        CProcessSemaphore *pProcessSemaphore = (CProcessSemaphore *)_pSemaphore;
        pthread_mutex_lock(&pProcessSemaphore->m_Mutex);
        ++pProcessSemaphore->m_Counter;
        pthread_cond_signal(&pProcessSemaphore->m_Cond);
        pthread_mutex_unlock(&pProcessSemaphore->m_Mutex);
#endif
    }
    
    
    const HPMString GetConfigValue(const HPMString& Key)
    {
        
        return m_ConfigMap[Key];
    }
    
    
    bool InitConnection()
    {
        if (m_pSession)
            return true;
        
        HPMUInt64 CurrentTime = GetTimeSince1970();
        if (CurrentTime > m_NextConnectionAttempt)
        {
            m_NextConnectionAttempt = 0;
            
            EHPMSdkDebugMode DebugMode = EHPMSdkDebugMode_Off;
            
#ifdef _MSC_VER
            m_ProcessCallbackInfo.m_pContext = m_ProcessSemaphore;
#elif __GNUC__
            m_ProcessCallbackInfo.m_pContext = &m_ProcessSemaphore;
#endif
            m_ProcessCallbackInfo.m_pCallback = &CHansoftSDKSample_Simple::NeedSessionProcessCallback;
            
            
            int ServerPort = stoi(m_ConfigMap[hpm_str("HansoftServerPort")]);
            
            try
            {
                m_pSession = HPMSdkSession::SessionOpen(GetConfigValue(hpm_str("HansoftServerAddress")), ServerPort, GetConfigValue(hpm_str("Database")), GetConfigValue(hpm_str("SDKUserName")), GetConfigValue(hpm_str("SDKUserPassword")), this, &m_ProcessCallbackInfo, true, DebugMode, NULL, 0, hpm_str(""), HPMSystemString(), NULL);
            }
            catch (const HPMSdkException &_Error)
            {
                HPMString SdkError = _Error.GetAsString();
                wstring Error(SdkError.begin(), SdkError.end());
                wcout << hpm_str("SessionOpen failed with error:") << Error << hpm_str("\r\n");
                return false;
            }
            catch (const HPMSdkCppException &_Error)
            {
                wcout << hpm_str("SessionOpen failed with error:") << _Error.what() << hpm_str("\r\n");
                return false;
            }
            
            wcout << "Successfully opened session.\r\n";
            m_bBrokenConnection = false;
            
            return true;
            
            
        }
        
        return false;
    }
    
    void DestroyConnection()
    {
        m_ChartSubscriptions.clear();
        
        if (m_pSession)
        {
            HPMSdkSession::SessionDestroy(m_pSession);
            m_pSession = NULL;
        }
        
        m_NextUpdate = 0; // Update when connection is restored
    }
    
    
    using HPMSdkCallbacks::On_Callback;
    
    map<HPMUniqueID, HPMDashboardChartSubscription> m_ChartSubscriptions;
    set<HPMUniqueID> m_UpdatedCharts;
    map< HPMString, HPMString> m_ConfigMap;
    
    template<typename Integer>
    HPMString ToHPMString(Integer Value)
    {
        HPMString ReturnValue;
        
#if HPMCharSize == 1
        ReturnValue = std::to_string(Value);
#else
        ReturnValue = std::to_wstring(Value);
#endif
        
        return ReturnValue;
    }
    
    void On_Callback(const HPMChangeCallbackData_DashboardChartReceive &_Data) override
    {
        m_UpdatedCharts.insert(_Data.m_ChartID);
        wcout << "Chart " << _Data.m_ChartID << " was updated.\r\n";
    }
    
    bool CreateDir(const string& DirectoryName)
    {
#ifdef _WIN32
        if (CreateDirectoryA(DirectoryName.c_str(), NULL))
            return true;
        else if (ERROR_ALREADY_EXISTS == GetLastError())
            return true;
        else
            return false;
#elif __GNUC__
        if(mkdir(DirectoryName.c_str(), 0777))
            return true;
        else if (EEXIST == errno)
            return true;
        else
            return false;
#endif
    }
    
    void ExportChart(HPMUniqueID ChartID, HPMString ResultSet)
    {
        bool IRGDirectory;
        IRGDirectory = CreateDir("IRG");
        HPMString ChartRegistryFileName;
        
        if (IRGDirectory)
        {
#ifdef _WIN32
            ChartRegistryFileName = hpm_str(".\\IRG\\Chart") + ToHPMString(ChartID.m_ID) + hpm_str(".irg");
#elif __GNUC__
            ChartRegistryFileName = hpm_str("./IRG/Chart") + ToHPMString(ChartID.m_ID) + hpm_str(".irg");
#endif
        }
        else
        {
            ChartRegistryFileName = hpm_str("Chart") + ToHPMString(ChartID.m_ID) + hpm_str(".irg");
        }
        
        HPMString ChartImageFileName = GetConfigValue(hpm_str("OutputFolderPath")) + hpm_str("Chart") + ToHPMString(ChartID.m_ID) + hpm_str(".") + GetConfigValue(hpm_str("OutputFormat"));
        
        // Write the result set registry to file
        std::filebuf FileBuffer;
        FileBuffer.open(ChartRegistryFileName.c_str(), std::ios::out);
        std::ostream OutStream(&FileBuffer);
        OutStream << string(ResultSet.begin(), ResultSet.end());
        FileBuffer.close();
        
        HPMString Resolution = GetConfigValue(hpm_str("OutputResolution"));
        
        HPMString ResultSetToolPath =
#ifdef _MSC_VER
        GetConfigValue(hpm_str("ResultSetToolPath"));
#elif __GNUC__
        GetConfigValue(hpm_str("ResultSetToolPath"));
#endif
        
        HPMString CommandLine = ResultSetToolPath + hpm_str(" ") + ChartRegistryFileName + hpm_str(" ") + ChartImageFileName + hpm_str(" ") + Resolution;
        
#if HPMCharSize == 1
        system(CommandLine.c_str());
#else
        _wsystem(CommandLine.c_str());
#endif
        
    }
    
    void SubscribeToChart(HPMUniqueID ChartID)
    {
        try
        {
            if (m_ChartSubscriptions.find(ChartID) == m_ChartSubscriptions.end())
                m_ChartSubscriptions[ChartID] = m_pSession->DashboardSubscribeToChart(ChartID);
        }
        catch (HPMSdkException &_Error)
        {
            cout << _Error.GetError();
        }
    }
    
    void SubscribeToPage(HPMUniqueID PageID)
    {
        HPMUniqueEnum ChartIDs = m_pSession->DashboardChartEnum(PageID);
        for (HPMUniqueID ChartID : ChartIDs.m_IDs)
            SubscribeToChart(ChartID);
    }
    
    HPMUniqueID FindChartByName(HPMUniqueID PageID, HPMString Name)
    {
        HPMUniqueEnum Charts = m_pSession->DashboardChartEnum(PageID);
        for (HPMUniqueID ChartID : Charts.m_IDs)
        {
            HPMString ChartName = m_pSession->DashboardChartGetName(ChartID);
            if (ChartName == Name)
                return ChartID;
        }
        return HPMUniqueID();
    }
    
    HPMUniqueID FindPageByName(HPMString Name)
    {
        HPMUniqueEnum Pages = m_pSession->DashboardPageEnum();
        for (HPMUniqueID PageID : Pages.m_IDs)
        {
            HPMString PageName = m_pSession->DashboardPageGetName(PageID);
            if (PageName == Name)
                return PageID;
        }
        return HPMUniqueID();
    }
    
    void Update()
    {
        if (InitConnection())
        {
            try
            {
                if (m_bBrokenConnection)
                {
                    DestroyConnection();
                }
                else
                {
                    HPMUInt64 CurrentTime = GetTimeSince1970();
                    if (CurrentTime > m_NextUpdate)
                    {
                        vector<HPMUniqueID> Charts;
                        bool ChartLoad = LoadCharts(Charts, GetConfigValue(hpm_str("ChartIDFilePath")));
                        if (ChartLoad)
                        {
                            HPMUniqueEnum Pages = m_pSession->DashboardPageEnum();
                            
                            for (size_t i = 0; i < Charts.size(); ++i)
                            {
                                if (find(Pages.m_IDs.begin(), Pages.m_IDs.end(), Charts[i]) != Pages.m_IDs.end())
                                {
                                    HPMUniqueEnum PageCharts = m_pSession->DashboardChartEnum(Charts[i]);
                                    for (size_t i = 0; i < PageCharts.m_IDs.size(); ++i)
                                    {
                                        SubscribeToChart(PageCharts.m_IDs[i]);
                                    }
                                }
                                else
                                {
                                    SubscribeToChart(Charts[i]);
                                }
                            }
                        }
                        
                        
#if (DEBUG)
                        m_NextUpdate = CurrentTime + 10000000; // Check every 10 seconds
                        #else
                            m_NextUpdate = CurrentTime + (stoi(GetConfigValue(hpm_str("RefreshRate"))) * 1000000);
#endif
                    }
                    
                    
                    m_UpdatedCharts.clear();
                    m_pSession->SessionProcess();
                    
                    if (!m_UpdatedCharts.empty())
                    {
                        for (HPMUniqueID ChartID : m_UpdatedCharts)
                        {
                            HPMString ResultSet = m_pSession->DashboardSubscriptionGetLastResultSetAsString(m_ChartSubscriptions[ChartID]);
                            ExportChart(ChartID, ResultSet);
                        }
                    }
                }
            }
            catch (HPMSdkException &_Error)
            {
                HPMString SdkError = _Error.GetAsString();
                wstring Error(SdkError.begin(), SdkError.end());
                wcout << hpm_str("Exception in processing loop: ") << Error << hpm_str("\r\n");
            }
            catch (HPMSdkCppException _Error)
            {
                wcout << hpm_str("Exception in processing loop: ") << _Error.what() << hpm_str("\r\n");
            }
        }
    }
    
    int Run()
    {
        // Initialize the SDK
        while (!_kbhit())
        {
            bool readConfig = ReadConfig(m_ConfigPath);
            
            if (!readConfig)
                return -1;
            
            if (!IsConfigValid())
                return -1;
            
            Update();
#ifdef _MSC_VER
            WaitForSingleObjectEx(m_ProcessSemaphore, 100, true);
#elif __GNUC__
            SemWait(100);
#endif
        }
        
        DestroyConnection();
        
        return 0;
    }
};

#ifdef _MSC_VER
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, const char * argv[])
#endif
{
    if (argc != 2)
    {
        cout << "Error: No config path provided or too many arguments." << endl;
        return 0;
    }
    HPMString ConfigPath = argv[1];
    
    CHansoftSDKSample_Simple Sample(ConfigPath);
    
    return Sample.Run();
}

