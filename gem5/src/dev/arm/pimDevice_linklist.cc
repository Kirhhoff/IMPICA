#include "config/pimDevice.hh"

#if (USE_PIM_DEVICE == PIM_DEVICE_LINK_LIST)

#include "dev/arm/pimDevice_linklist.hh"
#include "debug/PIM.hh"
#include "debug/PIMMMU.hh"
#include "base/bitfield.hh"

#define GRAB_PAGE_TABLE   (1)

/*
  PimPort::PimPort(MemObject *dev, System *s)
  : MasterPort(dev->name() + ".pim", dev), device(dev),
  sys(s), masterId(s->getMasterId(dev->name()))
  { }

  void PimPort::traverse(Addr phys)
  {
  DPRINTF(PIM, "In traverse!\n");

  Addr address=phys; 
  int size = 4;

  // creating a request
  Request *req = new Request(address,size, Request::UNCACHEABLE, masterId);
  req->taskId(ContextSwitchTaskId::Unknown);
  PacketPtr pkt = new Packet(req,MemCmd::ReadReq);
  // issue request
  sendTimingReq(pkt);
  }

  bool PimPort::recvTimingResp(PacketPtr pkt)
  {

  DPRINTF(PIM, "In recvTimingResp! the value of packet: %#X: \n",pkt->get<uint32_t>());
  if(findChild())
  {
  Addr phys = virtToPhys(pkt->get<>())
  traverse(phys);
  }
  else // findChild()==0 --> it means that this is the leaf node
  {
    
  }

  return true;

  }



  void PimPort::recvRetry()
  {
  }
*/


 
/*
 * With LPAE and 4KB pages, there are 3 levels of page tables. Each level has
 * 512 entries of 8 bytes each, occupying a 4K page. The first level table
 * covers a range of 512GB, each entry representing 1GB. The user and kernel
 * address spaces are limited to 512GB each.
 */
#define PTRS_PER_PTE		512
#define PTRS_PER_PMD		512
#define PTRS_PER_PUD		512
#define PTRS_PER_PGD		512

/*
 * PGDIR_SHIFT determines the size a top-level page table entry can map.
 */
#define PGDIR_SHIFT		38
#define PGDIR_SIZE		(1UL << PGDIR_SHIFT)
#define PGDIR_MASK		(~(PGDIR_SIZE-1))

/*
 * PUD_SHIFT determines the size a upper-level page table entry can map.
 */
#define PUD_SHIFT		30
#define PUD_SIZE		(1UL << PUD_SHIFT)
#define PUD_MASK		(~(PUD_SIZE-1))


/*
 * PMD_SHIFT determines the size a middle-level page table entry can map.
 */
#define PMD_SHIFT		21
#define PMD_SIZE		(1UL << PMD_SHIFT)
#define PMD_MASK		(~(PMD_SIZE-1))

/*
 * PMD_SHIFT determines the size a last-level page table entry can map.
 */
#define PTE_SHIFT		12
#define PTE_SIZE		(1UL << PTE_SHIFT)
#define PTE_MASK		(~(PTE_SIZE-1))

/*
 * Highest possible physical address supported.
 */
#define PHYS_MASK_SHIFT		(48)
#define PHYS_MASK           ((1UL << PHYS_MASK_SHIFT) - 1)

/*
 * Page table size 
 */
#define PAGE_MASK_SHIFT		(12)
#define PAGE_MASK           ((1UL << PAGE_MASK_SHIFT) - 1)

/* The shift of page table offset */
#define PTABLE_INDEX_OFFSET (3)

/* to find the page table base address with page descriptor */
#define pg_table_addr(pd) ((pd) & PHYS_MASK & ~PAGE_MASK)

/* to find an entry in a page-table-directory */
#define pgd_index(addr)		(((addr) >> PGDIR_SHIFT) & (PTRS_PER_PGD - 1))

#define pgd_offset(ttbr, addr) (ttbr+(pgd_index(addr)<<PTABLE_INDEX_OFFSET))

/* Find an entry in the first-level page table.. */
#define pud_index(addr)		(((addr) >> PUD_SHIFT) & (PTRS_PER_PUD - 1))

#define pud_offset(pgd, addr)	(pg_table_addr(pgd)+(pud_index(addr)<<PTABLE_INDEX_OFFSET))

/* Find an entry in the second-level page table.. */
#define pmd_index(addr)		(((addr) >> PMD_SHIFT) & (PTRS_PER_PMD - 1))

#define pmd_offset(pud, addr)	(pg_table_addr(pud)+(pmd_index(addr)<<PTABLE_INDEX_OFFSET))

/* Find an entry in the last-level page table.. */
#define pte_index(addr)		(((addr) >> PTE_SHIFT) & (PTRS_PER_PTE - 1))

#define pte_offset(pmd, addr)	(pg_table_addr(pmd)+(pte_index(addr)<<PTABLE_INDEX_OFFSET))


#define pd_none(pd)		(!(pd))
#define pd_bad(pd)		    (!((pd) & 1))
#define pd_table(pd)       (!!((pd) & 2))


PimDevice::PimDevice(Params *p)
	: AmbaDmaDevice(p), 
	  dmaDoneEventAll(maxOutstandingDma, this), dmaDoneEventFree(maxOutstandingDma),
      tlbEntryAll(p->pim_tlb_num)
{
	int i;

	pioSize = 0x1000;

	for (i = 0; i < maxOutstandingDma; ++i)
        dmaDoneEventFree[i] = &dmaDoneEventAll[i];

	for (i = 0; i < p->pim_tlb_num; ++i)
        tlbEntryList.push_back(&tlbEntryAll[i]);

	for (i = 0; i < PIM_DEVICE_CH; i++) {
		doingPageWalk[i] = false;
		startTrav[i] = false;
		doneTrav[i] = false;
		totalTravRequest[i] = 0;
	}

	isPageTableFetched = false;
	fetchPageTableIndex = 0;

	warn("The number of TLB entry : %d\n", p->pim_tlb_num);

}

PimDevice::~PimDevice()
{
	
}

Tick PimDevice::read(PacketPtr pkt)
{

	Addr daddr = pkt->getAddr() - pioAddr;
	uint64_t data = 0, ch;

	assert(pkt->getAddr() >= pioAddr &&
           pkt->getAddr() < pioAddr + pioSize);

	pkt->allocate();

    ch = (uint32_t)(daddr & ~ChannelMask) >> ChannelShft;
	daddr &= ChannelMask;

	assert(ch < PIM_DEVICE_CH);

	switch (daddr) {
	case RootAddrLo:
		data = rootAddr[ch];
		break;
	case RootAddrHi:
		data = (uint32_t)(rootAddr[ch] >> 32);
		break;
	case PageTableLo:
		data = pageTableAddr[ch];
		break;
	case PageTableHi:
		data = (uint32_t)(pageTableAddr[ch] >> 32);
		break;
    case KeyLo:
		data = key[ch];
		break;
	case KeyHi:
		data = (uint32_t)(key[ch] >> 32);
		break;
    case StartTrav:
		data = startTrav[ch];
		break;
	case Done:
		data = done;
		// data = doneTrav[ch];
		break;
	case ItemLo:
		data = item[ch];
		break;
	case ItemHi:
		data = (uint32_t)(item[ch] >> 32);
		break;
	case ItemIndex:
		data = itemIndex[ch];
		break;
	case LeafLo:
		data = leafAddr[ch];
		break;
	case LeafHi:
		data = (uint32_t)(leafAddr[ch] >> 32);
		break;
	case GrabPageTable:

#if (GRAB_PAGE_TABLE)
		data = (isPageTableFetched) ? 2 : 0;
#else
		data = 2;
#endif

		break;
	case Pthis:
		data = pthis;
		break;
	case Ptarget:
		data = ptarget;
		break;
	case Vthis:
		data = vthis;
		break;
	case Tsize:
		data = tsize;
		break;
	default:
		warn("Read to unsupported registers for PIM BT\n");
	    break;
	};

	switch(pkt->getSize()) {
	case 1:
        pkt->set<uint8_t>(data);
        break;
	case 2:
        pkt->set<uint16_t>(data);
        break;
	case 4:
        pkt->set<uint32_t>(data);
        break;
	case 8:
        pkt->set<uint64_t>(data);
        break;

	default:
        panic("PIM device read size too big?\n");
        break;
    }

	pkt->makeAtomicResponse();

	if (daddr != Done) {
		DPRINTF(PIM, "Device %s: Read request at offset %#X and channel %d with value %#X\n", 
				devname, daddr, ch, data);
	}

	//	warn("Device %s: Read request at offset %#X", devname, daddr);

	return pioDelay;

}

Tick PimDevice::write(PacketPtr pkt)
{

	Addr daddr = pkt->getAddr() - pioAddr;
	uint64_t data = 0, ch = 0;
	bool data_64b = false;

	switch(pkt->getSize()) {
	case 1:
        data = pkt->get<uint8_t>();
        break;
	case 2:
        data = pkt->get<uint16_t>();
        break;
	case 4:
        data = pkt->get<uint32_t>();
        break;
	case 8:
        data = pkt->get<uint64_t>();
	    data_64b = true;
        break;
	default:
        panic("PIM  write size too big?\n");
        break;
    }

    assert(pkt->getAddr() >= pioAddr &&
           pkt->getAddr() < pioAddr + pioSize);

	//Addr phys=0x00;
	// Translate virtual to phys address
	// Tanslate(pkt->get<uint32_t>());

	//    DPRINTF(PIM, "In writeFunction!\n");
	DPRINTF(PIM, "Device %s: Write request at offset %#X with value %#X\n", 
			devname, daddr, data);

    ch = (daddr & ~ChannelMask) >> ChannelShft;
	daddr &= ChannelMask;

	assert(ch < PIM_DEVICE_CH);

	switch (daddr) {
	case RootAddrLo:
		if (data_64b)
			rootAddr[ch] = data;
		else
			rootAddr[ch] = (rootAddr[ch] & mask(63, 32)) | data;
		break;
	case RootAddrHi:
		rootAddr[ch] = (rootAddr[ch] & mask(31, 0)) | (data << 32);
		break;
	case PageTableLo:
		if (data_64b)
			pageTableAddr[ch] = data;
		else
			pageTableAddr[ch] = (pageTableAddr[ch] & mask(63, 32)) | data;
		break;
	case PageTableHi:
		pageTableAddr[ch] = (pageTableAddr[ch] & mask(31, 0)) | (data << 32);
		//transVirtAddrToPhysAddr(rootAddr);
		break;
    case KeyLo:
		if (data_64b) {
			key[ch] = data;
			startBtreeTraverse(ch);   //optimization, start the traversal right after we get the key
		} else {
			key[ch] = (key[ch] & mask(63, 32)) | data;
		}
		break;
	case KeyHi:
		key[ch] = (key[ch] & mask(31, 0)) | (data << 32);
		break;
    case StartTrav:
		startBtreeTraverse(ch);
		break;
	case GrabPageTable:

#if (GRAB_PAGE_TABLE)
		startGrabPageTable();
#endif
		
		break;
	case Pthis:
		pthis = data;
		break;
	case Ptarget:
		ptarget = data;
		break;
	case Vthis:
		vthis = data;
		break;
	case Tsize:
		done = false;
		tsize = data;
		start_find(0);
		break;
	default:
		warn("Write to unsupported registers for PIM BT\n");
	    break;
	};

	pkt->makeAtomicResponse();

	return pioDelay;
}

// #define N_PIM_DEBUG

void PimDevice::start_find(uint64_t state) {
	DmaDoneEvent* event;
	
	switch (state)
	{
	case 0:
#ifndef N_PIM_DEBUG
		printf(	"[LOG:] PIM find starts:\n"
				"\tbegin.pa = [0x%lx]\n"
				"\ttarget.pa = [0x%lx]\n"
				"\tbegin.va = [0x%lx]\n"
				"\tT.size = [0x%lx]\n",
				pthis, ptarget, vthis, tsize);
#endif

		cur_vthis = vthis;
		tsize += 3 * PTR_SIZE;
		ext = 0;
		end_pos = 0;
		header_cnt = 0;
		header_read_finished = false;
		header_read_pos = 0;
		pread = ptarget & ~PIM_CROSS_PAGE;
		size = tsize;
		
read_target_loop:
#ifndef N_PIM_DEBUG
		printf(	"[LOG:] PIM state:\n"
				"\tcur.vthis = [0x%lx]\n"
				"\tT.size = [0x%lx]\n"
				"\tsize = [0x%lx]\n"
				"\theader.read_finished = [%d]\n"
				"\theader.cnt = [0x%lx]\n"
				"\theader.thismeta.idx = [0x%lx]\n",
				cur_vthis, tsize, size,
				header_read_finished, header_cnt, header_read_pos);
#endif

		readlen = min(size, PAGE_SIZE - (pread & PAGE_MASK));

#ifndef N_PIM_DEBUG
		printf(	"[LOG:] PIM read operation:\n"
				"\tread.addr = [0x%lx]\n"
				"\tread.len = [0x%lx]\n",
				pread, readlen);
#endif

		event = getFreeDmaDoneEvent();
		event->setEventTrigger(FetchTarget, 0);
		dmaPort.dmaAction(MemCmd::ReadReq, pread,
							readlen, event, 
							(uint8_t*)&buff[end_pos],
							50, Request::PIM_ACCESS);
		break;

	case 1:
#ifndef N_PIM_DEBUG
		printf("[LOG:] after read operation, target buff contents:\n");
		for (int i = 0; i < 12; i++) {
            printf("\t[%d] = [0x%lx]\n", i, buff[i]);
		}
#endif

		end_pos += readlen / PTR_SIZE;
		size -= readlen;
		
		if (!header_read_finished) {
            while (header_cnt < end_pos && (buff[header_cnt] & PIM_META_END) == 0) {
                header_cnt++;
                ext += PTR_SIZE;
            }

            printf("header cnt = [%ld] end pos = [%ld] size left = [%ld]\n", header_cnt, end_pos, size);
            
            if (header_cnt < end_pos) {
                header_cnt++;
                ext += PTR_SIZE;   
                header_read_finished = true;
                
				if (size == 0) {
                    size = ext;
                    tsize += ext;
                    pread += readlen;
                    goto read_target_loop;
                }
            }
            size += ext;
            tsize += ext;
            ext = 0; 
        }

        pread = buff[header_read_pos++] & ~(PIM_CROSS_PAGE | PIM_META_END);

		if (size > 0) {
			goto read_target_loop;
		}

read_node_loop:
#ifndef N_PIM_DEBUG
		printf(	"[LOG:] PIM read node starts:\n"
				"\tbegin.pa = [0x%lx]\n"
				"\ttarget.pa = [0x%lx]\n"
				"\tbegin.va = [0x%lx]\n"
				"\tT.size = [0x%lx]\n",
				pthis, ptarget, cur_vthis, tsize);
#endif

		if (pthis == 0) {
			goto not_found;
		}
		
		end_pos = 0;
        header_cnt = 0;
        header_read_finished = false;
        header_read_pos = 0;
        pread = pthis & ~PIM_CROSS_PAGE;
        size = tsize;

read_node_part_loop:
		readlen = min(size, PAGE_SIZE - (pread & PAGE_MASK));

        printf("target readlen = %ld\n", readlen);
        printf("target read addr = %p\n", reinterpret_cast<void*>(pread));

		event = getFreeDmaDoneEvent();
		event->setEventTrigger(FetchNode, 0);
		dmaPort.dmaAction(MemCmd::ReadReq, pread,
							readlen, event, 
							(uint8_t*)&cbuff[end_pos],
							50, Request::PIM_ACCESS);
		break;

	case 2:
        for (int i = 0; i < 12; i++) {
            printf("cbuff[%d] = 0x%lx\n", i, cbuff[i]);
        }

		end_pos += readlen / PTR_SIZE;
		size -= readlen;

        printf("totoal %ld\n", tsize);

		if (!header_read_finished) {
			while (header_cnt < end_pos && (cbuff[header_cnt] & PIM_META_END) == 0) {
				header_cnt++;
			}

            printf("header cnt = [%ld] end pos = [%ld] size left = [%ld]\n", header_cnt, end_pos, size);

			if (header_cnt < end_pos) {
				printf("header read finished\n");
				header_cnt++;
				header_read_finished = true;
				cmp_pos = header_cnt + 3;
				if (cmp_pos >= end_pos) {
					goto move_to_next_part;
				}
			} else {
				goto move_to_next_part;
			}
		}

		if (memcmp(&cbuff[cmp_pos], &buff[cmp_pos], (end_pos - cmp_pos) * PTR_SIZE) != 0) {
			goto move_to_next;
		}

		cmp_pos = end_pos;

move_to_next_part:
		pread = cbuff[header_read_pos++] & ~(PIM_CROSS_PAGE | PIM_META_END);

        printf("pread %p\n", reinterpret_cast<void*>(pread));

		if (size > 0)
			goto read_node_part_loop;

        if (cmp_pos == end_pos) {
			vthis = cur_vthis;
			done = true;
			return;
			// return vthis;
        }

move_to_next:
            pthis = cbuff[header_cnt];
            // vthis = cbuff[header_cnt + 1];
			cur_vthis = cbuff[header_cnt + 1];
			printf("move to next p = 0x%lx, v = 0x%lx\n", pthis, cur_vthis);
		goto read_node_loop;
	default:
		break;
	}

	return;

not_found:
	vthis = 0;
	done = true;
}

// 	start B-tree traversal
void PimDevice::startBtreeTraverse(uint64_t ch)
{
	assert(!startTrav[ch]);
	startTrav[ch] = true;
	doneTrav[ch] = false;
	
	//leafAddr[ch] = rootAddr[ch];
	//btNodeCrossPage[ch] = isBtNodeCrossPage(rootAddr[ch]);
	leafAddr[ch] = key[ch];
	btNodeCrossPage[ch] = isBtNodeCrossPage(key[ch]);
 
	totalRequest[ch]++;
	totalTravRequest[ch]++;
	// DEBUG DEBUG
	//assert(totalTravRequest[ch] <= 30);

	travRequestStart[ch] = curTick();

	// Translate the address of root to physical address.
	// After that, the BT node will be fetched automatically
	//transVirtAddrToPhysAddr(rootAddr[ch], ch);
	transVirtAddrToPhysAddr(key[ch], ch);
}

// start grab first level page table
void PimDevice::startGrabPageTable()
{
	DmaDoneEvent *event = getFreeDmaDoneEvent();

	event->setEventTrigger(FetchPageTable, 0);

	fetchPageTableIndex = 0;

	dmaPort.dmaAction(MemCmd::ReadReq, pageTableAddr[0],
		              sizeof(firstLevelPageTable), event, (uint8_t *) &firstLevelPageTable,
					  0, Request::PIM_ACCESS); //Request::UNCACHEABLE);
}

// grab second level page table
void PimDevice::grabSecondPageTable()
{
	if (fetchPageTableIndex >= (PTRS_PER_PUD)) {
		isPageTableFetched = true;
		DPRINTF(PIMMMU, "Grab second level page table done");
		return;
	}

	while (fetchPageTableIndex < PTRS_PER_PUD) {
	
		uint64_t pud_entry = firstLevelPageTable[fetchPageTableIndex];

		if (!pd_none(pud_entry) && !pd_bad(pud_entry) && pd_table(pud_entry)) {

			DmaDoneEvent *event = getFreeDmaDoneEvent();

			event->setEventTrigger(FetchPageTable, 0);
			
			dmaPort.dmaAction(MemCmd::ReadReq, 
							  pg_table_addr(pud_entry),
							  sizeof(secondLevelPageTable[0]), event, 
							  (uint8_t *) &secondLevelPageTable[fetchPageTableIndex],
							  0, Request::PIM_ACCESS); //Request::UNCACHEABLE);

			fetchPageTableIndex++;
			return;			
		}
		fetchPageTableIndex ++;
	}
	
	if (fetchPageTableIndex >= (PTRS_PER_PUD)) {
		isPageTableFetched = true;
		DPRINTF(PIMMMU, "Grab second level page table done");
		return;
	}
}



// whether the address of BT node exceeds a page
bool PimDevice::isBtNodeCrossPage(unsigned long addr)
{
	return ((addr & ~(PAGE_MASK)) != ((addr + sizeof(bt_node) - 1) & ~(PAGE_MASK)));
}

// the function to do VA to PA translation
uint64_t PimDevice::transVirtAddrToPhysAddr(uint64_t virtAddr, uint64_t ch)
{
	DPRINTF(PIMMMU, "Translate VA %#X with page table base %#X\n", virtAddr, pageTableAddr[ch]);

	assert(!doingPageWalk[ch]);

	doingPageWalk[ch] = true;
	virtualAddrToTrans[ch] = virtAddr;

	// Stats
	totalVaToPa++;
	vaToPaStart[ch] = curTick();
	totalVaToPaNum[ch]++;

	// Check TLB first
	std::list<PimTlb *>::iterator it;
	for (it = tlbEntryList.begin(); it != tlbEntryList.end(); ++it) {
		uint64_t pa;
		if ((*it)->isHit(virtAddr, &pa)) {
			// LRU, put this entry to front
			tlbEntryList.erase(it);
			tlbEntryList.push_front(*it);
			// Skip to page walk done
			pageWalkDone(pa, ch);
			totalTlbHit++;
			return 0;
		}
	}

	if (isPageTableFetched) {

		// Use the grabbed first level page table to go to the next level directly
		uint64_t pud_entry = firstLevelPageTable[pud_index(virtAddr)];
		firstLevelPageWalkDone(pud_entry, ch);

	} else {

		DmaDoneEvent *event = getFreeDmaDoneEvent();

		event->setEventTrigger(PageWalkLevel1, ch);    // The current kernel start from level 1

		dmaPort.dmaAction(MemCmd::ReadReq, pud_offset(pageTableAddr[ch], virtAddr), 
						  pageWalkDmaSize, event, (uint8_t *) &pageWalkDmaBuffer[ch],
						  0, Request::PIM_ACCESS); //Request::UNCACHEABLE);
	}

	return 0;
}

// insert a TLB entry
void PimDevice::insertTlbEntry(uint64_t virtAddr, uint64_t virtAddrSize, uint64_t physAddr)
{
	PimTlb *evictEntry;

	// Check if TLB entry already exists
	std::list<PimTlb *>::iterator it;
	for (it = tlbEntryList.begin(); it != tlbEntryList.end(); ++it) {
		uint64_t pa;
		if ((*it)->isHit(virtAddr, &pa)) {
			return;
		}
	}

	// Remove the entry in the back
	evictEntry = tlbEntryList.back();
	tlbEntryList.pop_back();

	// Set the entry to the new mapping, and put back to the front
	evictEntry->setEntry(virtAddr, virtAddrSize, physAddr);
	tlbEntryList.push_front(evictEntry);
}


// get a free DMA done event from the vector
PimDevice::DmaDoneEvent* PimDevice::getFreeDmaDoneEvent()
{
	assert(!dmaDoneEventFree.empty());
	DmaDoneEvent *event(dmaDoneEventFree.back());
	dmaDoneEventFree.pop_back();
	assert(!event->scheduled());

	return event;
}


// deal with the DMA done event for page table walk
void PimDevice::pageWalkDmaBack(uint64_t eventTrigger, uint64_t ch)
{
	DPRINTF(PIMMMU, "Page walker get data back for event %d, data = %#x, ch = %d\n", 
			eventTrigger, pageWalkDmaBuffer[ch], ch);

	assert(doingPageWalk[ch]);

    DmaDoneEvent *event;
	uint64_t descAddr, physAddr;

	switch (eventTrigger) {
	case PageWalkLevel0:
		// Level 0 page descriptor must be valid and point to a table
		// It can not be a block
		assert(!pd_none(pageWalkDmaBuffer[ch]) && !pd_bad(pageWalkDmaBuffer[ch]) &&
			   pd_table(pageWalkDmaBuffer[ch]));

		event = getFreeDmaDoneEvent();

		event->setEventTrigger(PageWalkLevel1, ch);

		descAddr = pud_offset(pageWalkDmaBuffer[ch], virtualAddrToTrans[ch]);

		DPRINTF(PIMMMU, "Go to level 1 table, addr = %#x\n", descAddr);

		dmaPort.dmaAction(MemCmd::ReadReq, descAddr, 
						  pageWalkDmaSize, event, (uint8_t *) &pageWalkDmaBuffer[ch],
						  0, Request::PIM_ACCESS); //Request::UNCACHEABLE);		
		break;
	case PageWalkLevel1:
		firstLevelPageWalkDone(pageWalkDmaBuffer[ch], ch);
		break;
	case PageWalkLevel2:
		secondLevelPageWalkDone(pageWalkDmaBuffer[ch], ch);
		break;
	case PageWalkLevel3:

		assert(!pd_none(pageWalkDmaBuffer[ch]) && !pd_bad(pageWalkDmaBuffer[ch]));
	  
		DPRINTF(PIMMMU, "Got page descriptor at level 3, PD = %#x\n", pageWalkDmaBuffer[ch]);
		insertTlbEntry(virtualAddrToTrans[ch] & PTE_MASK,
					   PTE_SIZE, pageWalkDmaBuffer[ch] & PTE_MASK & PHYS_MASK);
		physAddr = ((pageWalkDmaBuffer[ch]) & PTE_MASK & PHYS_MASK) 
			| (virtualAddrToTrans[ch] & ~PTE_MASK);
		pageWalkDone(physAddr, ch);
		
		break;
	default:
		assert(0);
	    break;
	}
}

// When first level page walk done, invoke this function
void PimDevice::firstLevelPageWalkDone(uint64_t pageTableEntry, uint64_t ch)
{
	assert(!pd_none(pageTableEntry) && !pd_bad(pageTableEntry));

	DmaDoneEvent *event;
	uint64_t virtAddr, descAddr, physAddr;

	if (!pd_table(pageTableEntry)) {
		DPRINTF(PIMMMU, "Got block at level 1, PD = %#x\n", pageTableEntry);
		insertTlbEntry(virtualAddrToTrans[ch] & PUD_MASK,
					   PUD_SIZE, pageTableEntry & PUD_MASK & PHYS_MASK);
		physAddr = (pageTableEntry & PUD_MASK & PHYS_MASK) 
			| (virtualAddrToTrans[ch] & ~PUD_MASK);
		pageWalkDone(physAddr, ch);
	} else {

		if (isPageTableFetched) {
			
			// Use the grabbed second level page table to go to the next level directly
			virtAddr = virtualAddrToTrans[ch];
			uint64_t pmd_entry = 
				secondLevelPageTable[pud_index(virtAddr)][pmd_index(virtAddr)];
			secondLevelPageWalkDone(pmd_entry, ch);

		} else {

			event = getFreeDmaDoneEvent();

			event->setEventTrigger(PageWalkLevel2, ch);

			descAddr = pmd_offset(pageTableEntry, virtualAddrToTrans[ch]);

			DPRINTF(PIMMMU, "Go to level 2 table, addr = %#x, ch = %d\n", descAddr, ch);

			dmaPort.dmaAction(MemCmd::ReadReq, descAddr, 
							  pageWalkDmaSize, event, (uint8_t *) &pageWalkDmaBuffer[ch],
							  0, Request::PIM_ACCESS); //Request::UNCACHEABLE);		
		}
	}
}

// When second level page walk done, invoke this function
void PimDevice::secondLevelPageWalkDone(uint64_t pageTableEntry, uint64_t ch)
{
	DmaDoneEvent *event;
	uint64_t descAddr, physAddr;
	
	assert(!pd_none(pageTableEntry) && !pd_bad(pageTableEntry));

	if (!pd_table(pageTableEntry)) {
		DPRINTF(PIMMMU, "Got block at level 2, PD = %#x\n", pageWalkDmaBuffer[ch]);
		insertTlbEntry(virtualAddrToTrans[ch] & PMD_MASK,
					   PMD_SIZE, pageTableEntry & PMD_MASK & PHYS_MASK);
		physAddr = ((pageTableEntry) & PMD_MASK & PHYS_MASK) 
			| (virtualAddrToTrans[ch] & ~PMD_MASK);
		pageWalkDone(physAddr, ch);
	} else {
		event = getFreeDmaDoneEvent();

		event->setEventTrigger(PageWalkLevel3, ch);

		descAddr = pte_offset(pageTableEntry, virtualAddrToTrans[ch]);

		DPRINTF(PIMMMU, "Go to level 3 table, addr = %#x\n", descAddr);

		dmaPort.dmaAction(MemCmd::ReadReq, descAddr, 
						  pageWalkDmaSize, event, (uint8_t *) &pageWalkDmaBuffer[ch],
						  0, Request::PIM_ACCESS); //Request::UNCACHEABLE);		
	}
}



// when page table walk has been done, invoke this function
void PimDevice::pageWalkDone(uint64_t physAddr, uint64_t ch)
{
	uint64_t fetchSize = 0;

	DPRINTF(PIMMMU, "Successfully translated VA %#x to PA %#x\n", 
			virtualAddrToTrans[ch], physAddr);

	physicalAddrTransalted[ch] = physAddr;

	doingPageWalk[ch] = false;
	totalVaToPaLatency[ch] += curTick() - vaToPaStart[ch];

	DmaDoneEvent *event;

	// Wakeup up the caller, here the only caller is BT traversal, 
    // so we fetch the BT node directly
	event = getFreeDmaDoneEvent();

	event->setEventTrigger(FetchBtNode, ch);

	DPRINTF(PIM, "Fetch BT node, addr = %lx, ch = %d\n", physAddr, ch);

	btNodeFetchStart[ch] = curTick();
	totalBtNodeNum[ch]++;

	if (!btNodeCrossPage[ch]) {
		dmaPort.dmaAction(MemCmd::ReadReq, physAddr, 
						  sizeof(bt_node), event, (uint8_t *) &btNodeDmaBuffer[ch],
						  0, Request::PIM_ACCESS);// Request::UNCACHEABLE);
	}
	else {
		if (isBtNodeCrossPage(physAddr)) {
			// The head of BT node that cross the page
			DPRINTF(PIM, "Fetch the head of BT node, ch =%d\n", ch);
			fetchSize = ((physAddr + PTE_SIZE) & PTE_MASK) - physAddr;
			btNodeCrossPageHeadSize[ch] = fetchSize;
			dmaPort.dmaAction(MemCmd::ReadReq, physAddr, 
							  fetchSize, event, (uint8_t *) &btNodeDmaBuffer[ch],
							  0, Request::PIM_ACCESS);// Request::UNCACHEABLE);
		}
		else {
			// The tail of BT node that cross the page
			DPRINTF(PIM, "Fetch the tail of BT node, ch =%d\n", ch);
			fetchSize = sizeof(bt_node) - btNodeCrossPageHeadSize[ch];			
			btNodeCrossPage[ch] = false;
			dmaPort.dmaAction(MemCmd::ReadReq, physAddr, 
							  fetchSize, event, 
							  ((uint8_t *) &btNodeDmaBuffer[ch]) + btNodeCrossPageHeadSize[ch],
							  0, Request::PIM_ACCESS);// Request::UNCACHEABLE);
		}
	}
}

// deal with the DMA done event for BT node fetch
void PimDevice::fetchBtNodeBack(uint64_t ch)
{
	//	int i, num_keys;

	DPRINTF(PIM, "Got BT node back at ch = %d, latency = %ld\n", ch, curTick() - btNodeFetchStart[ch]);
	totalBtNodeLatency[ch] += curTick() - btNodeFetchStart[ch];

	if (btNodeCrossPage[ch]) {
		unsigned long tailAddr = virtualAddrToTrans[ch] + btNodeCrossPageHeadSize[ch];
		// It's cross page. We need translate the address of tail 
		//DPRINTF(PIM, "Translate the tail of BT node at %lx, ch =%d\n", 
		//		tailAddr, ch);
		transVirtAddrToPhysAddr(tailAddr, ch);
		return;
	}

	if (btNodeDmaBuffer[ch].next == NULL) {
		startTrav[ch] = false;
		doneTrav[ch] = true;
		totalLatency[ch] += curTick() - travRequestStart[ch];
		DPRINTF(PIM, "Traverse to the tail node at %lx at ch %d\n", leafAddr[ch], ch);
	}
	else
    {
		// Go to get the child node, first we translate the VA to PA
		// After that, the BT node will be fetched automatically
		leafAddr[ch] = (uint64_t)btNodeDmaBuffer[ch].next;
		//DPRINTF(PIM, "Goto get BT node at %lx at ch %d\n", leafAddr[ch], ch);

		btNodeCrossPage[ch] = isBtNodeCrossPage(leafAddr[ch]);
		transVirtAddrToPhysAddr(leafAddr[ch], ch);

	}

	/*
	if (btNodeDmaBuffer[ch].num_keys == 0) {
		DPRINTF(PIM, "Got a node %lx without keys at channel %d, its parent is %lx\n", 
				virtualAddrToTrans[ch], ch, btNodeDmaBuffer[ch].parent);
		for (i = 0; i < BTREE_ORDER; i++) {
			DPRINTF(PIM, "Buggy Leaf key[%d] = %lx at ch %d\n", 
					i, btNodeDmaBuffer[ch].keys[i], ch);
			DPRINTF(PIM, "Buggy Leaf pointers[%d] = %lx at ch %d\n", 
					i, (unsigned long)btNodeDmaBuffer[ch].pointers[i], ch);
		}
	}
	
	num_keys = btNodeDmaBuffer[ch].num_keys;
	if (num_keys > BTREE_ORDER) {
		warn("Number of key is too large: %d at ch %d\n", num_keys, ch);
		num_keys = BTREE_ORDER;
	}
	if (btNodeDmaBuffer[ch].is_leaf) {
		for (i = 0; i < num_keys; i++) {
			//DPRINTF(PIM, "Leaf key[%d] = %lx at ch %d\n", i, btNodeDmaBuffer[ch].keys[i], ch);
			if (key[ch] == btNodeDmaBuffer[ch].keys[i]) {
				item[ch] = (uint64_t) btNodeDmaBuffer[ch].pointers[i];
				itemIndex[ch] = i;
				startTrav[ch] = false;
				doneTrav[ch] = true;
				totalLatency[ch] += curTick() - travRequestStart[ch];
				return;
			}
		}
		warn("Unable to find the key %ld for root %lx at channel %d, keynum = %d\n", 
			 key[ch], rootAddr[ch], ch, btNodeDmaBuffer[ch].num_keys);
		for (i = 0; i < num_keys; i++) {
			DPRINTF(PIM, "key[%d] = %lx\n", i, btNodeDmaBuffer[ch].keys[i]);
		}

		// for debug
		itemIndex[ch] = 0xFFFF;
		startTrav[ch] = false;
		doneTrav[ch] = true;
		
		//assert(0);
	}
	else {
		for (i = 0; i < num_keys; i++) {
			//DPRINTF(PIM, "Nonleaf key[%d] = %lx at ch %d\n", i, btNodeDmaBuffer[ch].keys[i], ch);
			//DPRINTF(PIM, "Nonleaf pointers[%d] = %lx at ch %d\n", i, 
			//		(uint64_t)btNodeDmaBuffer[ch].pointers[i], ch);
			if (key[ch] < btNodeDmaBuffer[ch].keys[i])
				break;
		}
		//if (i > 0)
		//	DPRINTF(PIM, "Nonleaf pre key[%d] = %ld\n", i-1, btNodeDmaBuffer[ch].keys[i-1]);
		//DPRINTF(PIM, "Nonleaf next key[%d] = %ld\n", i, btNodeDmaBuffer[ch].keys[i]);
		// Go to get the child node, first we translate the VA to PA
		// After that, the BT node will be fetched automatically
		leafAddr[ch] = (uint64_t)btNodeDmaBuffer[ch].pointers[i];
		//DPRINTF(PIM, "Goto get BT node at %lx at ch %d\n", leafAddr[ch], ch);

		btNodeCrossPage[ch] = isBtNodeCrossPage(leafAddr[ch]);
		transVirtAddrToPhysAddr(leafAddr[ch], ch);
	}
	*/
}


void PimDevice::dmaDone(uint64_t eventTrigger, uint64_t ch)
{
    DPRINTF(PIM, "DMA Done with event %d at ch %d\n", eventTrigger, ch);

	switch (eventTrigger) {
	case PageWalkLevel0:
	case PageWalkLevel1:
	case PageWalkLevel2:
	case PageWalkLevel3:
		pageWalkDmaBack(eventTrigger, ch);
		break;
    case FetchBtNode:
		fetchBtNodeBack(ch);
		break;
	case FetchPageTable:
		//isPageTableFetched = true;
		grabSecondPageTable();
		break;
	case FetchTarget:
	case FetchNode:
		start_find(eventTrigger - FetchBase);
	default:
		//		assert(0);
	    break;
	}
}


AddrRangeList PimDevice::getAddrRanges() const
{
    AddrRangeList ranges;
    ranges.push_back(RangeSize(pioAddr, pioSize));
	warn("PimDevice range: %#X %#X", pioAddr, pioSize);
    return ranges;
}

void PimDevice::serialize(std::ostream &os)
{
    DPRINTF(PIM, "Serializing PIM Device\n");
	
	SERIALIZE_ARRAY(rootAddr, PIM_DEVICE_CH);
	SERIALIZE_ARRAY(pageTableAddr, PIM_DEVICE_CH);
	SERIALIZE_ARRAY(key, PIM_DEVICE_CH);
	SERIALIZE_ARRAY(startTrav, PIM_DEVICE_CH);
	SERIALIZE_ARRAY(doneTrav, PIM_DEVICE_CH);
	SERIALIZE_ARRAY(item, PIM_DEVICE_CH);
	SERIALIZE_ARRAY(itemIndex, PIM_DEVICE_CH);
	SERIALIZE_ARRAY(leafAddr, PIM_DEVICE_CH);
	//	SERIALIZE_ARRAY(btNodeDmaBuffer, PIM_DEVICE_CH);
	SERIALIZE_ARRAY(virtualAddrToTrans, PIM_DEVICE_CH);
	SERIALIZE_ARRAY(physicalAddrTransalted, PIM_DEVICE_CH);
	SERIALIZE_ARRAY(pageWalkDmaBuffer, PIM_DEVICE_CH);
	SERIALIZE_ARRAY(doingPageWalk, PIM_DEVICE_CH);
	SERIALIZE_ARRAY(btNodeCrossPage, PIM_DEVICE_CH);
	SERIALIZE_ARRAY(btNodeCrossPageHeadSize, PIM_DEVICE_CH);
}

void PimDevice::unserialize(Checkpoint *cp, const std::string &section)
{
    DPRINTF(PIM, "Unserializing PIM Device\n");

	UNSERIALIZE_ARRAY(rootAddr, PIM_DEVICE_CH);
	UNSERIALIZE_ARRAY(pageTableAddr, PIM_DEVICE_CH);
	UNSERIALIZE_ARRAY(key, PIM_DEVICE_CH);
	UNSERIALIZE_ARRAY(startTrav, PIM_DEVICE_CH);
	UNSERIALIZE_ARRAY(doneTrav, PIM_DEVICE_CH);
	UNSERIALIZE_ARRAY(item, PIM_DEVICE_CH);
	UNSERIALIZE_ARRAY(itemIndex, PIM_DEVICE_CH);
	UNSERIALIZE_ARRAY(leafAddr, PIM_DEVICE_CH);
	//UNSERIALIZE_ARRAY(btNodeDmaBuffer, PIM_DEVICE_CH);
	UNSERIALIZE_ARRAY(virtualAddrToTrans, PIM_DEVICE_CH);
	UNSERIALIZE_ARRAY(physicalAddrTransalted, PIM_DEVICE_CH);
	UNSERIALIZE_ARRAY(pageWalkDmaBuffer, PIM_DEVICE_CH);
	UNSERIALIZE_ARRAY(doingPageWalk, PIM_DEVICE_CH);
	UNSERIALIZE_ARRAY(btNodeCrossPage, PIM_DEVICE_CH);
	UNSERIALIZE_ARRAY(btNodeCrossPageHeadSize, PIM_DEVICE_CH);
}

void PimDevice::regStats()
{
    using namespace Stats;

	totalVaToPa
        .name(name() + ".num_pim_va_to_pa")
        .desc("number of VA to PA translation for PIM")
        .flags(total | nozero | nonan)
        ;

	totalTlbHit
        .name(name() + ".num_pim_tlb_hit")
        .desc("number of TLB hit for PIM")
        .flags(total | nozero | nonan)
        ;

	tlbHitRate
        .name(name() + ".pim_tlb_hit_rate")
        .desc("TLB hit rate for PIM")
        .flags(total | nozero | nonan)
        ;
	tlbHitRate = totalTlbHit / totalVaToPa;

	totalRequest
        .init(PIM_DEVICE_CH)
        .name(name() + ".totalRequest")
        .desc("Total request for B-tree traversal");

	totalLatency
        .init(PIM_DEVICE_CH)
        .name(name() + ".totalLatency")
        .desc("Total latency for B-tree traversal");

	totalVaToPaLatency
        .init(PIM_DEVICE_CH)
        .name(name() + ".totalVaToPaLatency")
        .desc("Total latency for VA to PA translation");

	totalVaToPaNum
		.init(PIM_DEVICE_CH)
        .name(name() + ".totalVaToPaNum")
        .desc("Total access number for VA to PA translation");

	totalBtNodeLatency
        .init(PIM_DEVICE_CH)
        .name(name() + ".totalBtNodeLatency")
        .desc("Total latency for B-tree node fetching");

	totalBtNodeNum
		.init(PIM_DEVICE_CH)
        .name(name() + ".totalBtNodeNum")
        .desc("Total access number for B-tree node fetching");

	overallAvgLatency
        .name(name() + ".overallAvgLatency")
        .desc("Avg traversal latency")
        .flags(total | nozero | nonan)
        ;

	overallAvgLatency = totalLatency / totalRequest;

	overallAvgVaToPaLatency
        .name(name() + ".overallAvgVaToPaLatency")
        .desc("Avg VA to PA translation latency per traversal")
        .flags(total | nozero | nonan)
        ;

	overallAvgVaToPaLatency = totalVaToPaLatency / totalRequest;

	overallAvgBtNodeLatency
        .name(name() + ".overallAvgBtNodeLatency")
        .desc("Avg BT node fetching latency per traversal")
        .flags(total | nozero | nonan)
        ;

	overallAvgBtNodeLatency = totalBtNodeLatency / totalRequest;

	accAvgVaToPaLatency
        .name(name() + ".accAvgVaToPaLatency")
        .desc("Avg VA to PA translation latency per access")
        .flags(total | nozero | nonan)
        ;

	accAvgVaToPaLatency = totalVaToPaLatency / totalVaToPaNum;

	accAvgBtNodeLatency
        .name(name() + ".accAvgBtNodeLatency")
        .desc("Avg BT node fetching latency per access")
        .flags(total | nozero | nonan)
        ;

	accAvgBtNodeLatency = totalBtNodeLatency / totalBtNodeNum;
}



PimDevice *PimDeviceParams::create()
{
	return new PimDevice(this);
}


#endif
