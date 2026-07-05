#pragma once

#include <stack>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <functional>

#include "../Leet.Panther2K.Util/include/PantherLogger.h"

#define PartitionTypeCount 224
#define PartitionTypeCommonCount 16

namespace Leet::Panther2K::Util
{
	class Logger;
}

namespace Leet
{
	namespace WinParted
	{
		enum class WP_TABLE_TYPE
		{
			Unknown = 0,
			MBR = 1,
			GPT = 2,
			PMBR = 4,
			GPT_PMBR = GPT | PMBR,
			GPT_HMBR = GPT | MBR,
		};

		enum class WP_OPERATING_MODE
		{
			Unknown,
			MBR,
			GPT,
		};

		struct WP_DISK_INFO
		{
			unsigned int DiskNumber;
			wchar_t DiskPath[64];
			wchar_t DeviceName[256];
			unsigned int PartitionCount;
			MEDIA_TYPE MediaType;
			unsigned int SectorSize;
			unsigned long long SectorCount;
		};

		struct PartitionType
		{
			short gDiskType;
			GUID guid;
			const wchar_t* display_name;
		};

		struct WP_PART_DESCRIPTION
		{
			int PartitionNumber;
			short PartitionType;
			wchar_t PartitionSize[10];
			wchar_t FileSystem[10];
			const wchar_t* MountPoint;
		};

		struct WP_PART_LAYOUT
		{
			int PartitionCount;
			WP_PART_DESCRIPTION Partitions[1];
		};


		enum class WP_CALLBACK_TYPE {
			Error,
			YesNo,
		};

		enum class WP_CALLBACK_RESULT {
			Ok,
			Yes,
			No,
		};

		struct WP_CALLBACK_INFO
		{
			std::wstring_view message;
			WP_CALLBACK_TYPE type;
		};

		typedef WP_CALLBACK_RESULT (WpCallback)(WP_CALLBACK_INFO callbackInfo);

		class PartitionManager
		{
		public:

			class PartitionManagerObject
			{
			public:
				virtual ~PartitionManagerObject() = default;
			private:
				friend class PartitionManager;
				int id;
			};

			class DiskPartitionTable;

			class Disk : public PartitionManagerObject
			{
			public:
				/*WP_TABLE_TYPE GetPartitionTableType(WP_DISK_INFO* diskInfo);*/
				std::weak_ptr<DiskPartitionTable> OpenPartitionTable(WP_OPERATING_MODE mode)
				{
					if (mode != WP_OPERATING_MODE::GPT && mode != WP_OPERATING_MODE::MBR)
						return std::weak_ptr<DiskPartitionTable>();
					return m_manager.CreateObject<DiskPartitionTable>(m_diskInfo);
				}
				WP_DISK_INFO GetDiskInfo()
				{
					return m_diskInfo;
				}
			private:
				friend class PartitionManager;
				PartitionManager& m_manager;
				WP_DISK_INFO& m_diskInfo;
				HANDLE diskHandle;
				Disk(PartitionManager& m, WP_DISK_INFO& info) : m_manager(m), m_diskInfo(info) { };
			};

			class DiskPartitionTable : public PartitionManagerObject
			{
			public:

			private:
				friend class PartitionManager;
				PartitionManager& m_manager;
				WP_DISK_INFO& m_diskInfo;
				DiskPartitionTable(PartitionManager& m, WP_DISK_INFO& info) : m_manager(m), m_diskInfo(info) {};

				/*bool SavePartitionTableToDisk();
				bool AddPartition(PartitionInformation* partInfo, unsigned long long flags);
				bool DeletePartition(PartitionInformation* partInfo);
				HRESULT ApplyPartitionLayoutGPT(WP_PART_LAYOUT* layout);
				HRESULT ApplyPartitionLayoutMBR(WP_PART_LAYOUT* layout);*/
			};

			/// <summary>
			/// Opens a disk handle by its index number. 
			/// </summary>
			std::weak_ptr<Disk> OpenDisk(unsigned int diskIndex)
			{
				if (diskIndex >= m_diskInfos.size())
					return std::weak_ptr<Disk>();
				return CreateObject<Disk>(m_diskInfos[diskIndex]);
			}

		private:
			friend class Disk;
			int m_currentId = 0;
			std::vector<WP_DISK_INFO> m_diskInfos;
			std::unordered_map<int, std::shared_ptr<PartitionManagerObject>> m_openHandles;

			template <typename T, typename... Args> std::weak_ptr<T> CreateObject(Args&&... args) {
				std::shared_ptr<T> obj(new T(*this, std::forward<Args>(args)...));
				obj->id = m_currentId++;
				m_openHandles[obj->id] = obj;
				return obj;
			}

		public:

			// 
			// DI Helper classes
			//
			void SetLogger(Panther2K::Util::Logger* logger);
			void SetCallback(std::function<WP_CALLBACK_RESULT(const WP_CALLBACK_INFO&)> callback);

			static const PartitionType GptTypes[PartitionTypeCount];

			/// <summary>
			/// Reads all connected disks and updates the internal disk information table. This function invalidates ALL outstanding handles.
			/// </summary>
			void Refresh();

			PartitionManager(Panther2K::Util::Logger* logger)
			{
				this->logger = logger;
			}

			/*
			//
			// Partition table manipulation
			//
			static bool LoadPartitionTable();


			static DISK_INFORMATION* DiskInformationTable;
			static long DiskInformationTableSize;


			static DISK_INFORMATION CurrentDisk;
			static PartitionTableType CurrentDiskType;
			static MBR_HEADER CurrentDiskMBR;
			static GPT_HEADER CurrentDiskGPT;
			static GPT_ENTRY* CurrentDiskGPTTable;
			static OperatingMode CurrentDiskOperatingMode;
			static PartitionInformation* CurrentDiskPartitions;
			static long CurrentDiskPartitionCount;
			static bool CurrentDiskPartitionsModified;
			static bool CurrentDiskPartitionTableDestroyed;
			static long CurrentDiskFirstAvailablePartition;

			//
			// Partition manipulation
			//

			static bool LoadPartition(PartitionInformation* partition);
			static bool SetCurrentPartitionType(short value);
			static bool SetCurrentPartitionGuid(GUID value);

			static PartitionInformation CurrentPartition;

			//
			// Miscellaneous conversion stuff
			//

			static const wchar_t* GetOperatingModeString();
			static const wchar_t* GetOperatingModeExtraString();
			static void GetSizeStringFromBytes(unsigned long long bytes, wchar_t buffer[10]);
			static void GetGuidStringFromStructure(GUID bytes, wchar_t buffer[37]);
			static void GetGuidStructureFromString(GUID* bytes, const wchar_t buffer[37]);
			static const wchar_t* GetStringFromPartitionTypeGUID(GUID guid);
			static const wchar_t* GetStringFromPartitionSystemID(char systemID);
			static const wchar_t* GetStringFromPartitionTypeCode(short typeCode);
			static const GUID* GetGUIDFromPartitionTypeCode(short typeCode);
			static long CalculateCRC32(char* data, unsigned long long length);
			*/
		private:
			Leet::Panther2K::Util::Logger* logger;
		};
	}
}
