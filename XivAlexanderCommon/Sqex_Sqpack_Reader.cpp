#include "pch.h"
#include "Sqex_Sqpack_Reader.h"

Sqex::Sqpack::FileSystemSqPack::SqIndexType::SqIndexType(const Utils::Win32::File& hFile, bool strictVerify) {
	std::vector<std::pair<size_t, size_t>> accesses;

	hFile.Read(0, &Header, sizeof SqpackHeader);
	if (strictVerify) {
		accesses.emplace_back(0, Header.HeaderSize);
		Header.VerifySqpackHeader(SqpackType::SqIndex);
	}

	hFile.Read(Header.HeaderSize, &IndexHeader, sizeof Sqpack::SqIndex::Header);
	if (strictVerify) {
		accesses.emplace_back(Header.HeaderSize, IndexHeader.HeaderSize);
		IndexHeader.VerifySqpackIndexHeader(Sqpack::SqIndex::Header::IndexType::Index);
	}

	DataFileSegment.resize(IndexHeader.DataFilesSegment.Size);
	hFile.Read(IndexHeader.DataFilesSegment.Offset, std::span(DataFileSegment));
	if (strictVerify) {
		accesses.emplace_back(IndexHeader.DataFilesSegment.Offset, IndexHeader.DataFilesSegment.Size);
		IndexHeader.DataFilesSegment.Sha1.Verify(std::span(DataFileSegment), "DataFilesSegment Data SHA-1");
		IndexHeader.VerifyDataFileSegment(DataFileSegment);
	}

	Segment3.resize(IndexHeader.UnknownSegment3.Size / sizeof(Sqpack::SqIndex::Segment3Entry));
	hFile.Read(IndexHeader.UnknownSegment3.Offset, std::span(Segment3));
	if (strictVerify) {
		accesses.emplace_back(IndexHeader.UnknownSegment3.Offset, IndexHeader.UnknownSegment3.Size);
		IndexHeader.UnknownSegment3.Sha1.Verify(std::span(Segment3), "UnknownSegment3 Data SHA-1");
	}

	Folders.resize(IndexHeader.FolderSegment.Size / sizeof(Sqpack::SqIndex::FolderSegmentEntry));
	hFile.Read(IndexHeader.FolderSegment.Offset, std::span(Folders));
	if (strictVerify) {
		accesses.emplace_back(IndexHeader.FolderSegment.Offset, IndexHeader.FolderSegment.Size);
		IndexHeader.FolderSegment.Sha1.Verify(std::span(Folders), "FolderSegment Data SHA-1");
	}

	{
		uint32_t lastEnd = IndexHeader.FileSegment.Offset;
		for (const auto& folder : Folders) {
			if (strictVerify)
				folder.Verify();

			auto& filesInFolder = Files[folder.NameHash];
			filesInFolder.resize(folder.FileSegmentSize / sizeof(Sqpack::SqIndex::FileSegmentEntry));
			hFile.Read(folder.FileSegmentOffset, std::span(filesInFolder));

			if (folder.FileSegmentOffset >= IndexHeader.FileSegment.Offset &&
				folder.FileSegmentOffset < IndexHeader.FileSegment.Offset + IndexHeader.FileSegment.Size) {
				if (strictVerify) {
					accesses.emplace_back(folder.FileSegmentOffset, folder.FileSegmentSize);
					if (lastEnd != folder.FileSegmentOffset)
						throw CorruptDataException("last directory listing end != new directory listing start");
					for (const auto& file : filesInFolder)
						if (file.PathHash != folder.NameHash)
							throw CorruptDataException("file path hash != folder name hash");
				}
				lastEnd += folder.FileSegmentSize;
			} else if (folder.FileSegmentOffset >= IndexHeader.DataFilesSegment.Offset &&
				folder.FileSegmentOffset < IndexHeader.DataFilesSegment.Offset + IndexHeader.DataFilesSegment.Size) {
				Files.erase(folder.NameHash);
				// ignore for now
			} else if (folder.FileSegmentOffset >= IndexHeader.UnknownSegment3.Offset &&
				folder.FileSegmentOffset < IndexHeader.UnknownSegment3.Offset + IndexHeader.UnknownSegment3.Size) {
				Files.erase(folder.NameHash);
				// ignore for now
			}
		}

		if (strictVerify) {
			if (lastEnd != IndexHeader.FileSegment.Offset + IndexHeader.FileSegment.Size)
				throw CorruptDataException("last directory listing end != end of file segment");

			char result[20];
			CryptoPP::SHA1 sha1;
			for (const auto& files : Files | std::views::values)
				sha1.Update(reinterpret_cast<const byte*>(&files[0]), std::span(files).size_bytes());
			sha1.Final(reinterpret_cast<byte*>(result));
			if (IndexHeader.FileSegment.Sha1 != result) {
				if (IndexHeader.FileSegment.Size == 0 && IndexHeader.FileSegment.Sha1.IsZero()) {
					// pass
				} else
					throw CorruptDataException("FileSegment Data SHA-1");
			}
		}
	}

	if (strictVerify) {
		std::sort(accesses.begin(), accesses.end());
		auto ptr = accesses[0].first;
		for (const auto& [accessPointer, accessSize] : accesses) {
			if (ptr > accessPointer)
				throw CorruptDataException("Unread region found");
			else if (ptr < accessPointer)
				throw CorruptDataException("Overlapping region found");
			ptr += accessSize;
		}
		if (ptr != hFile.Length())
			throw CorruptDataException("Trailing region found");
	}
}

Sqex::Sqpack::FileSystemSqPack::SqIndex2Type::SqIndex2Type(const Utils::Win32::File& hFile, bool strictVerify) {
	std::vector<std::pair<size_t, size_t>> accesses;

	hFile.Read(0, &Header, sizeof SqpackHeader);
	if (strictVerify) {
		accesses.emplace_back(0, Header.HeaderSize);
		Header.VerifySqpackHeader(SqpackType::SqIndex);
	}

	hFile.Read(Header.HeaderSize, &IndexHeader, sizeof Sqpack::SqIndex::Header);
	accesses.emplace_back(Header.HeaderSize, IndexHeader.HeaderSize);
	if (strictVerify)
		IndexHeader.VerifySqpackIndexHeader(Sqpack::SqIndex::Header::IndexType::Index2);

	DataFileSegment.resize(IndexHeader.DataFilesSegment.Size);
	hFile.Read(IndexHeader.DataFilesSegment.Offset, std::span(DataFileSegment));
	if (strictVerify) {
		accesses.emplace_back(IndexHeader.DataFilesSegment.Offset, IndexHeader.DataFilesSegment.Size);
		IndexHeader.DataFilesSegment.Sha1.Verify(std::span(DataFileSegment), "DataFilesSegment Data SHA-1");
		IndexHeader.VerifyDataFileSegment(DataFileSegment);
	}

	Segment3.resize(IndexHeader.UnknownSegment3.Size / sizeof(Sqpack::SqIndex::Segment3Entry));
	hFile.Read(IndexHeader.UnknownSegment3.Offset, std::span(Segment3));
	if (strictVerify) {
		accesses.emplace_back(IndexHeader.UnknownSegment3.Offset, IndexHeader.UnknownSegment3.Size);
		IndexHeader.UnknownSegment3.Sha1.Verify(std::span(Segment3), "UnknownSegment3 Data SHA-1");
	}

	Folders.resize(IndexHeader.FolderSegment.Size / sizeof(Sqpack::SqIndex::FolderSegmentEntry));
	hFile.Read(IndexHeader.FolderSegment.Offset, std::span(Folders));
	if (strictVerify) {
		accesses.emplace_back(IndexHeader.FolderSegment.Offset, IndexHeader.FolderSegment.Size);
		IndexHeader.FolderSegment.Sha1.Verify(std::span(Folders), "FolderSegment Data SHA-1");
	}

	Files.resize(IndexHeader.FileSegment.Size / sizeof(Sqpack::SqIndex::FileSegmentEntry2));
	hFile.Read(IndexHeader.FileSegment.Offset, std::span(Files));
	if (strictVerify) {
		accesses.emplace_back(IndexHeader.FileSegment.Offset, IndexHeader.FileSegment.Size);
		IndexHeader.FileSegment.Sha1.Verify(std::span(Files), "FolderSegment Data SHA-1");
	}

	if (strictVerify) {
		std::sort(accesses.begin(), accesses.end());
		auto ptr = accesses[0].first;
		for (const auto& [accessPointer, accessSize] : accesses) {
			if (ptr > accessPointer)
				throw CorruptDataException("Unread region found");
			else if (ptr < accessPointer)
				throw CorruptDataException("Overlapping region found");
			ptr += accessSize;
		}
		if (ptr != hFile.Length())
			throw CorruptDataException("Trailing region found");
	}
}

Sqex::Sqpack::FileSystemSqPack::SqDataType::SqDataType(Utils::Win32::File hFile, const uint32_t datIndex, std::vector<SqDataEntry>& dataEntries, bool strictVerify)
	: FileOnDisk(std::move(hFile)) {
	std::vector<std::pair<size_t, size_t>> accesses;

	FileOnDisk.Read(0, &Header, sizeof SqpackHeader);
	if (strictVerify) {
		if (datIndex == 0)
			Header.VerifySqpackHeader(SqpackType::SqData);
		accesses.emplace_back(0, sizeof SqpackHeader);
	}

	FileOnDisk.Read(sizeof SqpackHeader, &DataHeader, sizeof Sqpack::SqData::Header);
	if (strictVerify) {
		DataHeader.Verify(datIndex + 1);
		accesses.emplace_back(sizeof SqpackHeader, sizeof Sqpack::SqData::Header);
	}

	const auto dataFileLength = FileOnDisk.Length();
	if (strictVerify) {
		if (dataFileLength != 0ULL + Header.HeaderSize + DataHeader.HeaderSize + DataHeader.DataSize)
			throw CorruptDataException("Invalid file size");
	}

	std::map<uint64_t, SqDataEntry*> offsetToEntryMap;
	for (auto& file : dataEntries) {
		if (file.IndexEntry.DatFile.Index() != datIndex)
			continue;
		offsetToEntryMap.insert_or_assign(file.IndexEntry.DatFile.Offset(), &file);
	}

	SqDataEntry* prevEntry = nullptr;
	for (const auto& [begin, entry] : offsetToEntryMap) {
		if (prevEntry)
			prevEntry->DataEntrySize = static_cast<uint32_t>(begin - prevEntry->IndexEntry.DatFile.Offset());
		prevEntry = entry;
	}
	if (prevEntry)
		prevEntry->DataEntrySize = static_cast<uint32_t>(dataFileLength - prevEntry->IndexEntry.DatFile.Offset());
}

Sqex::Sqpack::FileSystemSqPack::FileSystemSqPack(const std::filesystem::path& indexFile, bool strictVerify)
	: Index(Utils::Win32::File::Create(std::filesystem::path(indexFile).replace_extension(".index"), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN), strictVerify)
	, Index2(Utils::Win32::File::Create(std::filesystem::path(indexFile).replace_extension(".index2"), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN), strictVerify) {

	std::map<uint64_t, SqDataEntry*> offsetToEntryMap;

	Files.reserve(Index2.Files.size());
	for (const auto& entry : Index2.Files) {
		Files.emplace_back(SqDataEntry{
			.Index2Entry = entry,
			.DataEntryOffset = entry.DatFile.Offset(),
			.DataFileIndex = entry.DatFile.Index(),
			});
		offsetToEntryMap[entry.DatFile] = &Files.back();
	}

	std::vector<Sqpack::SqIndex::FileSegmentEntry*> newEntries;
	for (auto& files : Index.Files | std::views::values) {
		for (auto& entry : files) {
			const auto ptr = offsetToEntryMap[entry.DatFile];
			if (!ptr)
				newEntries.push_back(&entry);
			else
				ptr->IndexEntry = entry;
		}
	}
	for (const auto entry : newEntries)
		Files.emplace_back(SqDataEntry{
			.IndexEntry = *entry,
			.DataEntryOffset = entry->DatFile.Offset(),
			.DataFileIndex = entry->DatFile.Index(),
			});

	Data.reserve(Index.IndexHeader.DataFilesSegment.Count);
	for (uint32_t i = 0; i < Index.IndexHeader.DataFilesSegment.Count; ++i) {
		Data.emplace_back(SqDataType{
			Utils::Win32::File::Create(std::filesystem::path(indexFile).replace_extension(std::format(".dat{}", i)), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0),
			i,
			Files,
			strictVerify,
			});
	}
}