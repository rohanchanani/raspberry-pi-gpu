#include "rpi.h"
#include "fat32.h"
#include "fat32-helpers.h"
#include "pi-sd.h"

// Print extra tracing info when this is enabled.  You can and should add your
// own.
static int trace_p = 1; 
static int init_p = 0;

fat32_boot_sec_t boot_sector;


fat32_fs_t fat32_mk(mbr_partition_ent_t *partition) {
  demand(!init_p, "the fat32 module is already in use\n");
  // TODO: Read the boot sector (of the partition) off the SD card.
  fat32_boot_sec_t *boot_sector = pi_sec_read(partition->lba_start, 1);


  // TODO: Verify the boot sector (also called the volume id, `fat32_volume_id_check`)
  fat32_volume_id_check(boot_sector);

  // TODO: Read the FS info sector (the sector immediately following the boot
  // sector) and check it (`fat32_fsinfo_check`, `fat32_fsinfo_print`)
  assert(boot_sector->info_sec_num == 1);
  struct fsinfo info_sec;
  pi_sd_read(&info_sec, partition->lba_start+1, 1);
  fat32_fsinfo_check(&info_sec);

  // END OF PART 2
  // The rest of this is for Part 3:

  // TODO: calculate the fat32_fs_t metadata, which we'll need to return.
  unsigned lba_start = partition->lba_start; // from the partition
  unsigned fat_begin_lba = lba_start+boot_sector->reserved_area_nsec; // the start LBA + the number of reserved sectors
  unsigned cluster_begin_lba = fat_begin_lba+(boot_sector->nsec_per_fat*boot_sector->nfats); // the beginning of the FAT, plus the combined length of all the FATs
  unsigned sec_per_cluster = boot_sector->sec_per_cluster; // from the boot sector
  unsigned root_first_cluster = boot_sector->first_cluster; // from the boot sector
  unsigned n_entries = boot_sector->nsec_per_fat*512/4; // from the boot sector

  /*
   * TODO: Read in the entire fat (one copy: worth reading in the second and
   * comparing).
   *
   * The disk is divided into clusters. The number of sectors per
   * cluster is given in the boot sector byte 13. <sec_per_cluster>
   *
   * The File Allocation Table has one entry per cluster. This entry
   * uses 12, 16 or 28 bits for FAT12, FAT16 and FAT32.
   *
   * Store the FAT in a heap-allocated array.
   */
  uint32_t *fat;
  if (boot_sector->nsec_per_fat==0) {
  	fat = pi_sec_read(fat_begin_lba, 1);
  } else {
  	fat = pi_sec_read(fat_begin_lba, boot_sector->nsec_per_fat);
  }
  // Create the FAT32 FS struct with all the metadata
  fat32_fs_t fs = (fat32_fs_t) {
    .lba_start = lba_start,
      .fat_begin_lba = fat_begin_lba,
      .cluster_begin_lba = cluster_begin_lba,
      .sectors_per_cluster = sec_per_cluster,
      .root_dir_first_cluster = root_first_cluster,
      .fat = fat,
      .n_entries = n_entries,
  };

  if (trace_p) {
    trace("begin lba = %d\n", fs.fat_begin_lba);
    trace("cluster begin lba = %d\n", fs.cluster_begin_lba);
    trace("sectors per cluster = %d\n", fs.sectors_per_cluster);
    trace("root dir first cluster = %d\n", fs.root_dir_first_cluster);
  }

  init_p = 1;
  return fs;
}

// Given cluster_number, get lba.  Helper function.
static uint32_t cluster_to_lba(fat32_fs_t *f, uint32_t cluster_num) {
  assert(cluster_num >= 2);
  // TODO: calculate LBA from cluster number, cluster_begin_lba, and
  // sectors_per_cluster
  unsigned lba = f->cluster_begin_lba + (cluster_num-2)*f->sectors_per_cluster;
  if (trace_p) trace("cluster %d to lba: %d\n", cluster_num, lba);
  return lba;
}

pi_dirent_t fat32_get_root(fat32_fs_t *fs) {
  demand(init_p, "fat32 not initialized!");
  // TODO: return the information corresponding to the root directory (just
  // cluster_id, in this case)
  return (pi_dirent_t) {
    .name = "",
      .raw_name = "",
      .cluster_id = fs->root_dir_first_cluster,
      .is_dir_p = 1,
      .nbytes = 0,
  };
}


// Given the starting cluster index, get the length of the chain.  Helper
// function.
static uint32_t get_cluster_chain_length(fat32_fs_t *fs, uint32_t start_cluster) {
  // TODO: Walk the cluster chain in the FAT until you see a cluster where
  // `fat32_fat_entry_type(cluster) == LAST_CLUSTER`.  Count the number of
  // clusters.
  uint32_t num_clusters = 0;
  while (fat32_fat_entry_type(start_cluster) != LAST_CLUSTER) {
  	start_cluster = fs->fat[start_cluster];
	num_clusters++;
  }
  
  return num_clusters;
}

// Given the starting cluster index, read a cluster chain into a contiguous
// buffer.  Assume the provided buffer is large enough for the whole chain.
// Helper function.
static void read_cluster_chain(fat32_fs_t *fs, uint32_t start_cluster, uint8_t *data) {
  // TODO: Walk the cluster chain in the FAT until you see a cluster where
  // fat32_fat_entry_type(cluster) == LAST_CLUSTER.  For each cluster, copy it
  // to the buffer (`data`).  Be sure to offset your data pointer by the
  // appropriate amount each time.
  uint32_t num_clusters = 1;
  pi_sd_read(data, cluster_to_lba(fs, start_cluster), fs->sectors_per_cluster);
  start_cluster = fs->fat[start_cluster];
  while (fat32_fat_entry_type(start_cluster) != LAST_CLUSTER) {
	pi_sd_read(data + num_clusters*fs->sectors_per_cluster*512, cluster_to_lba(fs, start_cluster), fs->sectors_per_cluster);
	start_cluster = fs->fat[start_cluster];
	num_clusters++;
  }
}

// Converts a fat32 internal dirent into a generic one suitable for use outside
// this driver.
static pi_dirent_t dirent_convert(fat32_dirent_t *d) {
  pi_dirent_t e = {
    .cluster_id = fat32_cluster_id(d),
    .is_dir_p = d->attr == FAT32_DIR,
    .nbytes = d->file_nbytes,
  };
  // can compare this name
  memcpy(e.raw_name, d->filename, sizeof d->filename);
  // for printing.
  fat32_dirent_name(d,e.name);
  return e;
}

// Gets all the dirents of a directory which starts at cluster `cluster_start`.
// Return a heap-allocated array of dirents.
static fat32_dirent_t *get_dirents(fat32_fs_t *fs, uint32_t cluster_start, uint32_t *dir_n) {
  // TODO: figure out the length of the cluster chain (see
  // `get_cluster_chain_length`)
  unsigned length = get_cluster_chain_length(fs, cluster_start);

  // TODO: allocate a buffer large enough to hold the whole directory
  uint8_t *data = kmalloc(length*fs->sectors_per_cluster*512);
  *dir_n = length*fs->sectors_per_cluster*512/sizeof(fat32_dirent_t);
  // TODO: read in the whole directory (see `read_cluster_chain`)
  read_cluster_chain(fs, cluster_start, data);
  return (fat32_dirent_t *) data;
}

pi_directory_t fat32_readdir(fat32_fs_t *fs, pi_dirent_t *dirent) {
  demand(init_p, "fat32 not initialized!");
  demand(dirent->is_dir_p, "tried to readdir a file!");
  // use `get_dirents` to read the raw dirent structures from the disk
  uint32_t n_dirents;
  fat32_dirent_t *dirents = get_dirents(fs, dirent->cluster_id, &n_dirents);

  // allocate space to store the pi_dirent_t return values
  // unimplemented();
  pi_dirent_t * pi_dirents = kmalloc(n_dirents * sizeof(pi_dirent_t));
        
  // iterate over the directory and create pi_dirent_ts for every valid
  // file.  Don't include empty dirents, LFNs, or Volume IDs.  You can use
  // `dirent_convert`.
        
  int num_pi_dirents = 0;
  for(int i = 0; i < n_dirents; i++) {
    if (fat32_dirent_free(&dirents[i])) continue; // free space
    if (fat32_dirent_is_lfn(&dirents[i])) continue; // LFN version of name
    if (dirents[i].attr & FAT32_VOLUME_LABEL) continue; // volume label
    pi_dirent_t d = dirent_convert(&dirents[i]);
    pi_dirents[num_pi_dirents] = d;
    num_pi_dirents++;
  }
    
  printk("num_pi_dirents = %d\n", num_pi_dirents);
  
  // create a pi_directory_t using the dirents and the number of valid
  // dirents we found
  return (pi_directory_t) {
    .dirents = pi_dirents,
    .ndirents = num_pi_dirents,
  };
  
}

static int find_dirent_with_name(fat32_dirent_t *dirents, int n, char *filename) {
  // TODO: iterate through the dirents, looking for a file which matches the
  // name; use `fat32_dirent_name` to convert the internal name format to a
  // normal string.
  for (int i=0; i<n; i++) {
      if (fat32_dirent_free(&dirents[i])) {
      	continue;
      }
      char to_cmp[14];
      fat32_dirent_name(&dirents[i], to_cmp);
      if (strncmp(to_cmp, filename, strlen(filename))==0) {
           return i;
      }
  }
  return -1;
}

pi_dirent_t *fat32_stat(fat32_fs_t *fs, pi_dirent_t *directory, char *filename) {
  demand(init_p, "fat32 not initialized!");
  demand(directory->is_dir_p, "tried to use a file as a directory");

  // TODO: use `get_dirents` to read the raw dirent structures from the disk
  uint32_t num_entries;
  fat32_dirent_t *dirents = get_dirents(fs, directory->cluster_id, &num_entries);


  // TODO: Iterate through the directory's entries and find a dirent with the
  // provided name.  Return NULL if no such dirent exists.  You can use
  // `find_dirent_with_name` if you've implemented it.
  int result = find_dirent_with_name(dirents, num_entries, filename);

  if (result < 0) {
      return NULL;
  }
  // TODO: allocate enough space for the dirent, then convert
  // (`dirent_convert`) the fat32 dirent into a Pi dirent.
  
  pi_dirent_t *to_return = kmalloc(sizeof(pi_dirent_t));
  *to_return = dirent_convert(&dirents[result]);
  
  return to_return;
}

pi_file_t *fat32_read(fat32_fs_t *fs, pi_dirent_t *directory, char *filename) {
  // This should be pretty similar to readdir, but simpler.
  demand(init_p, "fat32 not initialized!");
  demand(directory->is_dir_p, "tried to use a file as a directory!");

  // TODO: read the dirents of the provided directory and look for one matching the provided name
  pi_dirent_t *found_file = fat32_stat(fs, directory, filename);


  // TODO: figure out the length of the cluster chain
  uint32_t length = get_cluster_chain_length(fs, found_file->cluster_id);

  // TODO: allocate a buffer large enough to hold the whole directory
  uint8_t *data = kmalloc(length*fs->sectors_per_cluster*512);

  // TODO: read in the whole directory (see `read_cluster_chain`)
  read_cluster_chain(fs, found_file->cluster_id, data);

  // TODO: fill the pi_file_t
  pi_file_t *file = kmalloc(sizeof(pi_file_t));
  *file = (pi_file_t) {
    .data = (char *)data,
    .n_data = found_file->nbytes,
    .n_alloc = length*fs->sectors_per_cluster*512,
  };
  return file;
}

/******************************************************************************
 * Everything below here is for writing to the SD card (Part 7/Extension).  If
 * you're working on read-only code, you don't need any of this.
 ******************************************************************************/

static uint32_t find_free_cluster(fat32_fs_t *fs, uint32_t start_cluster) {
  // TODO: loop through the entries in the FAT until you find a free one
  // (fat32_fat_entry_type == FREE_CLUSTER).  Start from cluster 3.  Panic if
  // there are none left.
  if (start_cluster < 3) start_cluster = 3;
  for (uint32_t test_cluster = start_cluster; test_cluster < fs->n_entries; test_cluster++) {
      if (fat32_fat_entry_type(fs->fat[test_cluster])==FREE_CLUSTER) {
          return test_cluster;
      }
  }
  if (trace_p) trace("failed to find free cluster from %d\n", start_cluster);
  panic("No more clusters on the disk!\n");
}

static void write_fat_to_disk(fat32_fs_t *fs) {
  // TODO: Write the FAT to disk.  In theory we should update every copy of the
  // FAT, but the first one is probably good enough.  A good OS would warn you
  // if the FATs are out of sync, but most OSes just read the first one without
  // complaining.
  if (trace_p) trace("syncing FAT\n");
  pi_sd_write(fs->fat, fs->fat_begin_lba, fs->n_entries*4/512);

}

// Given the starting cluster index, write the data in `data` over the
// pre-existing chain, adding new clusters to the end if necessary.
static void write_cluster_chain(fat32_fs_t *fs, uint32_t start_cluster, uint8_t *data, uint32_t nbytes) {
  // Walk the cluster chain in the FAT, writing the in-memory data to the
  // referenced clusters.  If the data is longer than the cluster chain, find
  // new free clusters and add them to the end of the list.
  // Things to consider:
  //  - what if the data is shorter than the cluster chain?
  //  - what if the data is longer than the cluster chain?
  //  - the last cluster needs to be marked LAST_CLUSTER
  //  - when do we want to write the updated FAT to the disk to prevent
  //  corruption?
  //  - what do we do when nbytes is 0?
  //  - what about when we don't have a valid cluster to start with?
  //
  //  This is the main "write" function we'll be using; the other functions
  //  will delegate their writing operations to this.

  // TODO: As long as we have bytes left to write and clusters to write them
  // to, walk the cluster chain writing them out.
  uint32_t last_cluster = 0;
  uint32_t curr_cluster = start_cluster;
  uint32_t num_written = 0;
  while (nbytes>0) {
	uint32_t num_sectors;
	if (nbytes<(512*fs->sectors_per_cluster)) {
	    num_sectors = (nbytes+511) & ~(511);
	    nbytes = 0;
	} else {
	    num_sectors = fs->sectors_per_cluster;
	    nbytes -= 512*fs->sectors_per_cluster;
	}
	if (fat32_fat_entry_type(curr_cluster) != USED_CLUSTER) {
	    panic("BAD CLUSTER\n");
	}
        pi_sd_write(data + num_written*fs->sectors_per_cluster*512, cluster_to_lba(fs, curr_cluster), num_sectors);
        num_written++;
	last_cluster = curr_cluster;	
	if (fat32_fat_entry_type(fs->fat[curr_cluster]) == USED_CLUSTER) {
            curr_cluster = fs->fat[curr_cluster];
	} else {
	    curr_cluster = find_free_cluster(fs, curr_cluster);
	    fs->fat[curr_cluster] = 1;
	    fs->fat[last_cluster] = curr_cluster;
	}
  }
  if (last_cluster> 0) {
  	fs->fat[last_cluster] = LAST_CLUSTER;
  }
  while (fat32_fat_entry_type(fs->fat[curr_cluster]==USED_CLUSTER)) {
  	uint32_t next_cluster = fs->fat[curr_cluster];
	fs->fat[curr_cluster] = 0;
	curr_cluster = next_cluster;
  }
  fs->fat[curr_cluster] = 0;

  // TODO: If we run out of clusters to write to, find free clusters using the
  // FAT and continue writing the bytes out.  Update the FAT to reflect the new
  // cluster.

  // TODO: If we run out of bytes to write before using all the clusters, mark
  // the final cluster as "LAST_CLUSTER" in the FAT, then free all the clusters
  // later in the chain.

  // TODO: Ensure that the last cluster in the chain is marked "LAST_CLUSTER".
  // The one exception to this is if we're writing 0 bytes in total, in which
  // case we don't want to use any clusters at all.
}

int fat32_rename(fat32_fs_t *fs, pi_dirent_t *directory, char *oldname, char *newname) {
  // TODO: Get the dirents `directory` off the disk, and iterate through them
  // looking for the file.  When you find it, rename it and write it back to
  // the disk (validate the name first).  Return 0 in case of an error, or 1
  // on success.
  // Consider:
  //  - what do you do when there's already a file with the new name?
  demand(init_p, "fat32 not initialized!");
  if (trace_p) trace("renaming %s to %s\n", oldname, newname);
  if (!fat32_is_valid_name(newname)) return 0;

  // TODO: get the dirents and find the right one
  uint32_t result;
  fat32_dirent_t *entries = get_dirents(fs, directory->cluster_id, &result);
  int found = find_dirent_with_name(entries, result, oldname);
  if (found==-1) {
	trace("Couldn't find %s\n", oldname);
  	return 0;
  }
  if (find_dirent_with_name(entries, result, newname) != -1) {
  	trace("%s already exists\n", newname);
	return 0;
  }

  // TODO: update the dirent's name
  fat32_dirent_set_name(&entries[found], newname);

  // TODO: write out the directory, using the existing cluster chain (or
  // appending to the end); implementing `write_cluster_chain` will help
  write_cluster_chain(fs, directory->cluster_id, (uint8_t *) entries, result*sizeof(fat32_dirent_t));
  return 1;

}

// Create a new directory entry for an empty file (or directory).
pi_dirent_t *fat32_create(fat32_fs_t *fs, pi_dirent_t *directory, char *filename, int is_dir) {
  demand(init_p, "fat32 not initialized!");
  if (trace_p) trace("creating %s\n", filename);
  if (!fat32_is_valid_name(filename)) return NULL;

  // TODO: read the dirents and make sure there isn't already a file with the
  // same name
  uint32_t result;
  fat32_dirent_t *entries = get_dirents(fs, directory->cluster_id, &result);
  if (find_dirent_with_name(entries, result, filename) != -1) {
      trace("%s EXISTS\n", filename);
      return NULL;
  }
  int found = -1;
  for (int i=0; i<result; i++) {
       if (fat32_dirent_free(&entries[i])) {
           found = i;
	   break;
       }
  }
  if (found==-1) {
	trace("No free directory entries available\n");
        return NULL;
  }
  // TODO: look for a free directory entry and use it to store the data for the
  // new file.  If there aren't any free directory entries, either panic or
  // (better) handle it appropriately by extending the directory to a new
  // cluster.
  // When you find one, update it to match the name and attributes
  // specified; set the size and cluster to 0.

  fat32_dirent_set_name(&entries[found], filename);
  char tmp[13];
  fat32_dirent_name(&entries[found], tmp);
  trace("Reading name %s\n", tmp);
  if (is_dir) {
      entries[found].attr = FAT32_DIR;
  } else {
      entries[found].attr = 0;
  }
  entries[found].lo_start = 0;
  entries[found].hi_start = 0;
  entries[found].file_nbytes = 0;
  // TODO: write out the updated directory to the disk
  write_cluster_chain(fs, directory->cluster_id, (uint8_t *) entries, result*sizeof(fat32_dirent_t));

  // TODO: convert the dirent to a `pi_dirent_t` and return a (kmalloc'ed)
  // pointer
  
  pi_dirent_t *dirent = kmalloc(sizeof(pi_dirent_t));
  *dirent = dirent_convert(&entries[found]);
  return dirent;
}

// Delete a file, including its directory entry.
int fat32_delete(fat32_fs_t *fs, pi_dirent_t *directory, char *filename) {
  demand(init_p, "fat32 not initialized!");
  if (trace_p) trace("deleting %s\n", filename);
  if (!fat32_is_valid_name(filename)) return 0;

  uint32_t result;
  fat32_dirent_t *entries = get_dirents(fs, directory->cluster_id, &result);
  int found = find_dirent_with_name(entries, result, filename);
  if  (found == -1) {
      return 0;
  }


  entries[found].filename[0] = 0xe5;
  uint8_t data[1];
  write_cluster_chain(fs, dirent_convert(&entries[found]).cluster_id, data, 0);
  // TODO: write out the updated directory to the disk
  write_cluster_chain(fs, directory->cluster_id, (uint8_t *) entries, result*sizeof(fat32_dirent_t));


  // TODO: look for a matching directory entry, and set the first byte of the
  // name to 0xE5 to mark it as free

  // TODO: free the clusters referenced by this dirent

  // TODO: write out the updated directory to the disk
  return 1;
}

int fat32_truncate(fat32_fs_t *fs, pi_dirent_t *directory, char *filename, unsigned length) {
  demand(init_p, "fat32 not initialized!");
  if (trace_p) trace("truncating %s\n", filename);

  // TODO: edit the directory entry of the file to list its length as `length` bytes,
  // then modify the cluster chain to either free unused clusters or add new
  // clusters.
  //
  // Consider: what if the file we're truncating has length 0? what if we're
  // truncating to length 0?
  uint32_t result;
  fat32_dirent_t *entries = get_dirents(fs, directory->cluster_id, &result);
  int found = find_dirent_with_name(entries, result, filename);
  if (found==-1) {
        return 0;
  }

  pi_dirent_t entry = dirent_convert(&entries[found]);
  uint32_t size;
  if (length > entry.nbytes+512*fs->sectors_per_cluster) {
  	size = length;
  } else {
  	size = entry.nbytes+512*fs->sectors_per_cluster;
  }
  uint8_t data[size];
  read_cluster_chain(fs, entry.cluster_id, data);

  write_cluster_chain(fs, entry.cluster_id, data, length);
  // TODO: write out the updated directory to the disk
  entries[found].file_nbytes = length;
  write_cluster_chain(fs, directory->cluster_id, (uint8_t *) entries, result*sizeof(fat32_dirent_t));


  // TODO: look for a matching directory entry, and set the first byte of the
  // name to 0xE5 to mark it as free

  // TODO: free the clusters referenced by this dirent

  // TODO: write out the updated directory to the disk


  // TODO: write out the directory entry
  return 1;
}

int fat32_write(fat32_fs_t *fs, pi_dirent_t *directory, char *filename, pi_file_t *file) {
  demand(init_p, "fat32 not initialized!");
  demand(directory->is_dir_p, "tried to use a file as a directory!");

  // TODO: Surprisingly, this one should be rather straightforward now.
  // - load the directory
  // - exit with an error (0) if there's no matching directory entry
  // - update the directory entry with the new size
  // - write out the file as clusters & update the FAT
  // - write out the directory entry
  // Special case: the file is empty to start with, so we need to update the
  // start cluster in the dirent
  uint32_t result;
  fat32_dirent_t *entries = get_dirents(fs, directory->cluster_id, &result);
  int found = find_dirent_with_name(entries, result, filename);
  if (found==-1){;
        return 0;
  }


  if (entries[found].file_nbytes==0) {
      uint32_t new_cluster = find_free_cluster(fs, 3);
      fs->fat[new_cluster] = 1;
      entries[found].lo_start = new_cluster & 0xffff;
      entries[found].hi_start = new_cluster >> 16;
  }

  write_cluster_chain(fs, dirent_convert(&entries[found]).cluster_id, file->data, file->n_data);
  entries[found].file_nbytes = file->n_data;
  // TODO: write out the updated directory to the disk
  write_cluster_chain(fs, directory->cluster_id, (uint8_t *) entries, result*sizeof(fat32_dirent_t));
  
  return 1;
}

int fat32_flush(fat32_fs_t *fs) {
  demand(init_p, "fat32 not initialized!");
  // no-op
  return 0;
}
