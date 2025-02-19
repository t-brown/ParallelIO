/**
 * @file
 * Private headers and defines for the PIO C interface.
 * @author Jim Edwards, Ed Hartnett
 * @date  2014
 *
 * @see https://github.com/NCAR/ParallelIO
 */

#ifndef __PIO_INTERNAL__
#define __PIO_INTERNAL__

#include <config.h>
#include <pio.h>
#include <pio_error.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <netcdf.h>
#ifdef _NETCDF4
#include <netcdf_par.h>
#include <netcdf_filter.h>
#include <netcdf_meta.h>
#endif
#ifdef _PNETCDF
#include <pnetcdf.h>
#endif
#ifdef TIMING
#include <gptl.h>
#endif
#include <assert.h>
#ifdef USE_MPE
#include <mpe.h>
#endif /* USE_MPE */

/* define an MPI type equivalent to size_t */
#if SIZE_MAX == UCHAR_MAX
   #define PIO_MPI_SIZE_T MPI_UNSIGNED_CHAR
#elif SIZE_MAX == USHRT_MAX
   #define PIO_MPI_SIZE_T MPI_UNSIGNED_SHORT
#elif SIZE_MAX == UINT_MAX
   #define PIO_MPI_SIZE_T MPI_UNSIGNED
#elif SIZE_MAX == ULONG_MAX
   #define PIO_MPI_SIZE_T MPI_UNSIGNED_LONG
#elif SIZE_MAX == ULLONG_MAX
   #define PIO_MPI_SIZE_T MPI_UNSIGNED_LONG_LONG
#else
   #error "what is happening here?"
#endif


//#ifndef MPI_OFFSET
/** MPI_OFFSET is an integer type of size sufficient to represent the
 * size (in bytes) of the largest file supported by MPI. In some MPI
 * implementations MPI_OFFSET is not properly defined.  */
//#define MPI_OFFSET  MPI_LONG_LONG
//#endif

/* These are the sizes of types in netCDF files. Do not replace these
 * constants with sizeof() calls for C types. They are not the
 * same. Even on a system where sizeof(short) is 4, the size of a
 * short in a netCDF file is 2 bytes. */
/** Size (in bytes) of a char in a netCDF file. */
#define NETCDF_CHAR_SIZE 1
/** Size (in bytes) of a short in a netCDF file. */
#define NETCDF_SHORT_SIZE 2
/** Size (in bytes) of a int or float in a netCDF file. */
#define NETCDF_INT_FLOAT_SIZE 4
/** Size (in bytes) of a long long int or double in a netCDF file. */
#define NETCDF_DOUBLE_INT64_SIZE 8

/* It seems that some versions of openmpi fail to define
 * MPI_OFFSET. */
#ifdef OMPI_OFFSET_DATATYPE
#ifndef MPI_OFFSET
#define MPI_OFFSET OMPI_OFFSET_DATATYPE
#endif
#endif
//#ifndef MPI_Offset
/** This is the type used for PIO_Offset. */
//#define MPI_Offset long long
//#endif

/** Some MPI implementations do not allow passing MPI_DATATYPE_NULL to
 * comm functions even though the send or recv length is 0, in these
 * cases we use MPI_CHAR, after this issue raised its ugly head again in mpich
 * 4.0.0 we decided to use this workaround in all cases.
 * See https://github.com/NCAR/ParallelIO/issues/1945 */

#define PIO_DATATYPE_NULL MPI_CHAR


#if PIO_ENABLE_LOGGING
void pio_log(int severity, const char *fmt, ...);
#define PLOG(e) pio_log e
#else
/** Logging macro for debugging. */
#define PLOG(e)
#endif /* PIO_ENABLE_LOGGING */

/** Find maximum. */
#define max(a,b)                                \
    ({ __typeof__ (a) _a = (a);                 \
        __typeof__ (b) _b = (b);                \
        _a > _b ? _a : _b; })

/** Find minimum. */
#define min(a,b)                                \
    ({ __typeof__ (a) _a = (a);                 \
        __typeof__ (b) _b = (b);                \
        _a < _b ? _a : _b; })

/** Block size of gathers. */
#define MAX_GATHER_BLOCK_SIZE 0

/** Request allocation size. */
#define PIO_REQUEST_ALLOC_CHUNK 16

/** This is needed to handle _long() functions. It may not be used as
 * a data type when creating attributes or varaibles, it is only used
 * internally. */
#define PIO_LONG_INTERNAL 13

#ifdef USE_MPE
/* These are for the event numbers array used to log various events in
 * the program with the MPE library, which produces output for the
 * Jumpshot program. */

/* Each event has start and end. */
#define START 0
#define END 1

/* These are the MPE states (events) we keep track of. */
#define NUM_EVENTS 7
#define INIT 0
#define DECOMP 1
#define CREATE 2
#define OPEN 3
#define DARRAY_WRITE 4
#define DARRAY_READ 6
#define CLOSE 5

/* The max length of msg added to log with mpe_log_pack(). (NULL
 * terminator is not required by mpe_log_pack(), so need not be
 * counted in this total).*/
#define MPE_MAX_MSG_LEN 32
#endif /* USE_MPE */

#if defined(__cplusplus)
extern "C" {
#endif

    extern PIO_Offset pio_pnetcdf_buffer_size_limit;

    /** Used to sort map points in the subset rearranger. */
    typedef struct mapsort
    {
        int rfrom; /**< from */
        PIO_Offset soffset; /**< ??? */
        PIO_Offset iomap; /**< ??? */
    } mapsort;

    /** swapm defaults. */
    typedef struct pio_swapm_defaults
    {
        int nreqs; /**< number of requests */
        bool handshake; /**< handshake */
        bool isend; /**< is end? */
    } pio_swapm_defaults;

    /* Handle an error in the PIO library. */
    int pio_err(iosystem_desc_t *ios, file_desc_t *file, int err_num, const char *fname,
                int line);

    /* Print error message and abort. */
    void piodie(const char *msg, const char *fname, int line);

    /* Assert that an expression is true. */
    void pioassert(_Bool expression, const char *msg, const char *fname, int line);

    /* Check the return code from an MPI function call. */
    int check_mpi(iosystem_desc_t *ios, file_desc_t *file, int mpierr, const char *filename,
                  int line);

    /* Check the return code from a netCDF call. */
    int check_netcdf(file_desc_t *file, int status, const char *fname, int line);

    /* Check the return code from a netCDF call, with ios pointer. */
    int check_netcdf2(iosystem_desc_t *ios, file_desc_t *file, int status,
                      const char *fname, int line);

    /* For async cases, this runs on IO tasks and listens for messages. */
    int pio_msg_handler2(int io_rank, int component_count, iosystem_desc_t **iosys,
                         MPI_Comm io_comm);

    /* List operations for iosystem list. */
    int pio_add_to_iosystem_list(iosystem_desc_t *ios);
    int pio_delete_iosystem_from_list(int piosysid);
    iosystem_desc_t *pio_get_iosystem_from_id(int iosysid);

    /* List operations for decomposition list. */
    int  pio_add_to_iodesc_list(io_desc_t *iodesc);
    io_desc_t *pio_get_iodesc_from_id(int ioid);
    int pio_delete_iodesc_from_list(int ioid);
    int pio_num_iosystem(int *niosysid);

    /* Allocate and initialize storage for decomposition information. */
    int malloc_iodesc(iosystem_desc_t *ios, int piotype, int ndims, io_desc_t **iodesc);

    /* List operations for file_desc_t list. */
    int pio_get_file(int ncid, file_desc_t **filep);
    int pio_delete_file_from_list(int ncid);
    void pio_add_to_file_list(file_desc_t *file);

    /* List operations for var_desc_t list. */
    int add_to_varlist(int varid, int rec_var, int pio_type, int pio_type_size,
                       MPI_Datatype mpi_type, int mpi_type_size, int ndim,
                       var_desc_t **varlist);
    int get_var_desc(int varid, var_desc_t **varlist, var_desc_t **var_desc);
    int delete_var_desc(int varid, var_desc_t **varlist);

    /* Create a file. */
    int PIOc_createfile_int(int iosysid, int *ncidp, int *iotype, const char *filename,
                            int mode, int use_ext_ncid);

    /* Open a file with optional retry as netCDF-classic if first
     * iotype does not work. */
    int PIOc_openfile_retry(int iosysid, int *ncidp, int *iotype, const char *filename, int mode,
                            int retry, int use_ext_ncid);

    /* Give the mode flag from an open, determine the IOTYPE. */
    int find_iotype_from_omode(int mode, int *iotype);

    /* Give the mode flag from an nc_create call, determine the IOTYPE. */
    int find_iotype_from_cmode(int cmode, int *iotype);

    /* Given PIO type, find MPI type and type size. */
    int find_mpi_type(int pio_type, MPI_Datatype *mpi_type, int *type_size);

    /* Check whether an IO type is valid for this build. */
    int iotype_is_valid(int iotype);

    /* Compute start and count values for each io task for a decomposition. */
    int CalcStartandCount(int pio_type, int ndims, const int *gdims, int num_io_procs,
                          int myiorank, PIO_Offset *start, PIO_Offset *count, int *num_aiotasks);

    /* Completes the mapping for the box rearranger. */
    int compute_counts(iosystem_desc_t *ios, io_desc_t *iodesc, const int *dest_ioproc,
                       const PIO_Offset *dest_ioindex);

    /* Create the MPI communicators needed by the subset rearranger. */
    int default_subset_partition(iosystem_desc_t *ios, io_desc_t *iodesc);

    /* Like MPI_Alltoallw(), but with flow control. */
    int pio_swapm(void *sendbuf, int *sendcounts, int *sdispls, MPI_Datatype *sendtypes,
                  void *recvbuf, int *recvcounts, int *rdispls, MPI_Datatype *recvtypes,
                  MPI_Comm comm, rearr_comm_fc_opt_t *fc);

    /* Return the greatest common devisor of array ain as int_64. */
    long long lgcd_array(int nain, long long* ain);

    /* Look for the largest block of data for io which can be
     * expressed in terms of start and count. */
    PIO_Offset GCDblocksize(int arrlen, const PIO_Offset *arr_in);

    /* Convert an index into dimension values. */
    void idx_to_dim_list(int ndims, const int *gdims, PIO_Offset idx, PIO_Offset *dim_list);

    /* Convert a global coordinate value into a local array index. */
    PIO_Offset coord_to_lindex(int ndims, const PIO_Offset *lcoord, const PIO_Offset *count);

    /* Determine whether fill values are needed. */
    int determine_fill(iosystem_desc_t *ios, io_desc_t *iodesc, const int *gsize,
                       const PIO_Offset *compmap);

    /* Allocation memory for a data region. */
    int alloc_region2(iosystem_desc_t *ios, int ndims, io_region **region);

    /* Set start and count so that they describe the first region in map.*/
    int find_region(int ndims, const int *gdims, int maplen, const PIO_Offset *map,
                    PIO_Offset *start, PIO_Offset *count, PIO_Offset *regionlen);

    /* Calculate start and count regions for the subset rearranger. */
    int get_regions(int ndims, const int *gdimlen, int maplen, const PIO_Offset *map,
                    int *maxregions, io_region *firstregion);

    /* Expand a region along dimension dim, by incrementing count[i] as
     * much as possible, consistent with the map. */
    void expand_region(int dim, const int *gdims, int maplen, const PIO_Offset *map,
                       int region_size, int region_stride, const int *max_size,
                       PIO_Offset *count);

    /* Free a region list. */
    void free_region_list(io_region *top);

    /* Create a subset rearranger. */
    int subset_rearrange_create(iosystem_desc_t *ios, int maplen, PIO_Offset *compmap, const int *gsize,
                                int ndim, io_desc_t *iodesc);

    /* Create a box rearranger. */
    int box_rearrange_create(iosystem_desc_t *ios, int maplen, const PIO_Offset *compmap, const int *gsize,
                             int ndim, io_desc_t *iodesc);

    /* Move data from IO tasks to compute tasks. */
    int rearrange_io2comp(iosystem_desc_t *ios, io_desc_t *iodesc, void *sbuf, void *rbuf);

    /* Move data from compute tasks to IO tasks. */
    int rearrange_comp2io(iosystem_desc_t *ios, io_desc_t *iodesc, void *sbuf, void *rbuf,
                          int nvars);

    void performance_tune_rearranger(iosystem_desc_t *ios, io_desc_t *iodesc);

    /* Flush contents of multi-buffer to disk. */
    int flush_output_buffer(file_desc_t *file, bool force, PIO_Offset addsize);

    int compute_maxaggregate_bytes(iosystem_desc_t *ios, io_desc_t *iodesc);

    /* Compute the size that the IO tasks will need to hold the data. */
    int compute_maxIObuffersize(MPI_Comm io_comm, io_desc_t *iodesc);

    /* Find greatest commond divisor. */
    int gcd(int a, int b);

    /* Find greatest commond divisor for long long. */
    long long lgcd (long long a, long long b );

    /* Convert a global coordinate value into a local array index. */
    PIO_Offset coord_to_lindex(int ndims, const PIO_Offset *lcoord, const PIO_Offset *count);

    /* Returns the smallest power of 2 greater than or equal to i. */
    int ceil2(int i);

    /* ??? */
    int pair(int np, int p, int k);

    /* Create MPI datatypes used for comp2io and io2comp data transfers. */
    int define_iodesc_datatypes(iosystem_desc_t *ios, io_desc_t *iodesc);

    /* Create the derived MPI datatypes used for comp2io and io2comp
     * transfers. */
    int create_mpi_datatypes(MPI_Datatype basetype, int msgcnt, const PIO_Offset *mindex,
                             const int *mcount, int *mfrom, MPI_Datatype *mtype);

    /* Used by subset rearranger to sort map. */
    int compare_offsets(const void *a, const void *b) ;

    /* Print a trace statement, for debugging. */
    void print_trace (FILE *fp);

    /* Flush PIO's data buffer. */
    int flush_buffer(int ncid, wmulti_buffer *wmb, bool flushtodisk);

    /* Compute an element of start/count arrays. */
    void compute_one_dim(int gdim, int ioprocs, int rank, PIO_Offset *start,
                         PIO_Offset *count);

    /* Darray support functions. */

    /* Write aggregated arrays to file using parallel I/O (netCDF-4 parallel/pnetcdf) */
    int write_darray_multi_par(file_desc_t *file, int nvars, int fndims, const int *vid,
                               io_desc_t *iodesc, int fill, const int *frame);

    /* Write aggregated arrays to file using serial I/O (netCDF-3/netCDF-4 serial) */
    int write_darray_multi_serial(file_desc_t *file, int nvars, int fndims, const int *vid,
                                  io_desc_t *iodesc, int fill, const int *frame);

    int pio_read_darray_nc(file_desc_t *file, io_desc_t *iodesc, int vid, void *iobuf);
    int pio_read_darray_nc_serial(file_desc_t *file, io_desc_t *iodesc, int vid, void *iobuf);
    int find_var_fillvalue(file_desc_t *file, int varid, var_desc_t *vdesc);

    /* Read atts with type conversion. */
    int PIOc_get_att_tc(int ncid, int varid, const char *name, nc_type memtype, void *ip);

    /* Write atts with type conversion. */
    int PIOc_put_att_tc(int ncid, int varid, const char *name, nc_type atttype,
                        PIO_Offset len, nc_type memtype, const void *op);

    /* Generalized get functions. */
    int PIOc_get_vars_tc(int ncid, int varid, const PIO_Offset *start, const PIO_Offset *count,
                         const PIO_Offset *stride, nc_type xtype, void *buf);
    int PIOc_get_var1_tc(int ncid, int varid, const PIO_Offset *index, nc_type xtype,
                         void *buf);
    int PIOc_get_var_tc(int ncid, int varid, nc_type xtype, void *buf);
    int PIOc_get_vard_tc(int ncid, int varid, int decompid, const PIO_Offset recnum,
                         nc_type xtype, void *buf);

    /* Generalized put functions. */
    int PIOc_put_vars_tc(int ncid, int varid, const PIO_Offset *start, const PIO_Offset *count,
                         const PIO_Offset *stride, nc_type xtype, const void *buf);
    int PIOc_put_var1_tc(int ncid, int varid, const PIO_Offset *index, nc_type xtype,
                         const void *op);
    int PIOc_put_var_tc(int ncid, int varid, nc_type xtype, const void *op);
    int PIOc_put_vard_tc(int ncid, int varid, int decompid, const PIO_Offset recnum,
                         nc_type xtype, const void *buf);

    /* An internal replacement for a function pnetcdf does not
     * have. */
    int pioc_pnetcdf_inq_type(int ncid, nc_type xtype, char *name,
                              PIO_Offset *sizep);

    /* Handle end and re-defs. */
    int pioc_change_def(int ncid, int is_enddef);

    /* Initialize and finalize logging, use --enable-logging at configure. */
    int pio_init_logging(void);
    void pio_finalize_logging(void );

#ifdef USE_MPE
    /* Logging with the MPE library, use --enable-mpe at configure. */
    void pio_start_mpe_log(int state);
    void pio_stop_mpe_log(int state, const char *msg);
#endif /* USE_MPE */

    /* Write a netCDF decomp file. */
    int pioc_write_nc_decomp_int(iosystem_desc_t *ios, const char *filename, int cmode, int ndims,
                                 int *global_dimlen, int num_tasks, int *task_maplen, int *map,
                                 const char *title, const char *history, int fortran_order);

    /* Read a netCDF decomp file. */
    int pioc_read_nc_decomp_int(int iosysid, const char *filename, int *ndims, int **global_dimlen,
                                int *num_tasks, int **task_maplen, int *max_maplen, int **map, char *title,
                                char *history, char *source, char *version, int *fortran_order);

    /* Determine what tasks to use for each computational component. */
    int determine_procs(int num_io_procs, int component_count, int *num_procs_per_comp,
                        int **proc_list, int **my_proc_list);

    int pio_sorted_copy(const void *array, void *tmparray, io_desc_t *iodesc, int nvars, int direction);

    int PIOc_inq_att_eh(int ncid, int varid, const char *name, int eh,
                        nc_type *xtypep, PIO_Offset *lenp);

    /* Start a timer. */
    int pio_start_timer(const char *name);

    /* Stop a timer. */
    int pio_stop_timer(const char *name);

    bool check_compmap(iosystem_desc_t *ios, io_desc_t *iodesc,const PIO_Offset *compmap);


#if defined(__cplusplus)
}
#endif

/** These are the messages that can be sent over the intercomm when
 * async is being used. */
enum PIO_MSG
{
    PIO_MSG_NULL,
    PIO_MSG_OPEN_FILE,
    PIO_MSG_CREATE_FILE,
    PIO_MSG_INQ_ATT,
    PIO_MSG_INQ_FORMAT,
    PIO_MSG_INQ_VARID,
    PIO_MSG_DEF_VAR,
    PIO_MSG_INQ_VAR,
    PIO_MSG_PUT_ATT_DOUBLE,
    PIO_MSG_PUT_ATT_INT,
    PIO_MSG_RENAME_ATT,
    PIO_MSG_DEL_ATT,
    PIO_MSG_INQ,
    PIO_MSG_GET_ATT_TEXT,
    PIO_MSG_GET_ATT_SHORT,
    PIO_MSG_PUT_ATT_LONG,
    PIO_MSG_REDEF,
    PIO_MSG_SET_FILL,
    PIO_MSG_ENDDEF,
    PIO_MSG_RENAME_VAR,
    PIO_MSG_PUT_ATT_SHORT,
    PIO_MSG_PUT_ATT_TEXT,
    PIO_MSG_INQ_ATTNAME,
    PIO_MSG_GET_ATT_ULONGLONG,
    PIO_MSG_GET_ATT_USHORT,
    PIO_MSG_PUT_ATT_ULONGLONG,
    PIO_MSG_GET_ATT_UINT,
    PIO_MSG_GET_ATT_LONGLONG,
    PIO_MSG_PUT_ATT_SCHAR,
    PIO_MSG_PUT_ATT_FLOAT,
    PIO_MSG_RENAME_DIM,
    PIO_MSG_GET_ATT_LONG,
    PIO_MSG_INQ_DIM,
    PIO_MSG_INQ_DIMID,
    PIO_MSG_PUT_ATT_USHORT,
    PIO_MSG_GET_ATT_FLOAT,
    PIO_MSG_SYNC,
    PIO_MSG_PUT_ATT_LONGLONG,
    PIO_MSG_PUT_ATT_UINT,
    PIO_MSG_GET_ATT_SCHAR,
    PIO_MSG_INQ_ATTID,
    PIO_MSG_DEF_DIM,
    PIO_MSG_GET_ATT_INT,
    PIO_MSG_GET_ATT_DOUBLE,
    PIO_MSG_PUT_ATT_UCHAR,
    PIO_MSG_GET_ATT_UCHAR,
    PIO_MSG_PUT_VARS_UCHAR,
    PIO_MSG_GET_VAR1_SCHAR,
    PIO_MSG_GET_VARS_ULONGLONG,
    PIO_MSG_GET_VARM_UCHAR,
    PIO_MSG_GET_VARM_SCHAR,
    PIO_MSG_GET_VARS_SHORT,
    PIO_MSG_GET_VAR_DOUBLE,
    PIO_MSG_GET_VARA_DOUBLE,
    PIO_MSG_GET_VAR_INT,
    PIO_MSG_GET_VAR_USHORT,
    PIO_MSG_PUT_VARS_USHORT,
    PIO_MSG_GET_VARA_TEXT,
    PIO_MSG_PUT_VARS_ULONGLONG,
    PIO_MSG_GET_VARA_INT,
    PIO_MSG_PUT_VARM,
    PIO_MSG_GET_VAR1_FLOAT,
    PIO_MSG_GET_VAR1_SHORT,
    PIO_MSG_GET_VARS_INT,
    PIO_MSG_PUT_VARS_UINT,
    PIO_MSG_GET_VAR_TEXT,
    PIO_MSG_GET_VARM_DOUBLE,
    PIO_MSG_PUT_VARM_UCHAR,
    PIO_MSG_PUT_VAR_USHORT,
    PIO_MSG_GET_VARS_SCHAR,
    PIO_MSG_GET_VARA_USHORT,
    PIO_MSG_PUT_VAR1_LONGLONG,
    PIO_MSG_PUT_VARA_UCHAR,
    PIO_MSG_PUT_VARM_SHORT,
    PIO_MSG_PUT_VAR1_LONG,
    PIO_MSG_PUT_VARS_LONG,
    PIO_MSG_GET_VAR1_USHORT,
    PIO_MSG_PUT_VAR_SHORT,
    PIO_MSG_PUT_VARA_INT,
    PIO_MSG_GET_VAR_FLOAT,
    PIO_MSG_PUT_VAR1_USHORT,
    PIO_MSG_PUT_VARA_TEXT,
    PIO_MSG_PUT_VARM_TEXT,
    PIO_MSG_GET_VARS_UCHAR,
    PIO_MSG_GET_VAR,
    PIO_MSG_PUT_VARM_USHORT,
    PIO_MSG_GET_VAR1_LONGLONG,
    PIO_MSG_GET_VARS_USHORT,
    PIO_MSG_GET_VAR_LONG,
    PIO_MSG_GET_VAR1_DOUBLE,
    PIO_MSG_PUT_VAR_ULONGLONG,
    PIO_MSG_PUT_VAR_INT,
    PIO_MSG_GET_VARA_UINT,
    PIO_MSG_PUT_VAR_LONGLONG,
    PIO_MSG_GET_VARS_LONGLONG,
    PIO_MSG_PUT_VAR_SCHAR,
    PIO_MSG_PUT_VAR_UINT,
    PIO_MSG_PUT_VAR,
    PIO_MSG_PUT_VARA_USHORT,
    PIO_MSG_GET_VAR_LONGLONG,
    PIO_MSG_GET_VARA_SHORT,
    PIO_MSG_PUT_VARS_SHORT,
    PIO_MSG_PUT_VARA_UINT,
    PIO_MSG_PUT_VARA_SCHAR,
    PIO_MSG_PUT_VARM_ULONGLONG,
    PIO_MSG_PUT_VAR1_UCHAR,
    PIO_MSG_PUT_VARM_INT,
    PIO_MSG_PUT_VARS_SCHAR,
    PIO_MSG_GET_VARA_LONG,
    PIO_MSG_PUT_VAR1,
    PIO_MSG_GET_VAR1_INT,
    PIO_MSG_GET_VAR1_ULONGLONG,
    PIO_MSG_GET_VAR_UCHAR,
    PIO_MSG_PUT_VARA_FLOAT,
    PIO_MSG_GET_VARA_UCHAR,
    PIO_MSG_GET_VARS_FLOAT,
    PIO_MSG_PUT_VAR1_FLOAT,
    PIO_MSG_PUT_VARM_FLOAT,
    PIO_MSG_PUT_VAR1_TEXT,
    PIO_MSG_PUT_VARS_TEXT,
    PIO_MSG_PUT_VARM_LONG,
    PIO_MSG_GET_VARS_LONG,
    PIO_MSG_PUT_VARS_DOUBLE,
    PIO_MSG_GET_VAR1,
    PIO_MSG_GET_VAR_UINT,
    PIO_MSG_PUT_VARA_LONGLONG,
    PIO_MSG_GET_VARA,
    PIO_MSG_PUT_VAR_DOUBLE,
    PIO_MSG_GET_VARA_SCHAR,
    PIO_MSG_PUT_VAR_FLOAT,
    PIO_MSG_GET_VAR1_UINT,
    PIO_MSG_GET_VARS_UINT,
    PIO_MSG_PUT_VAR1_ULONGLONG,
    PIO_MSG_PUT_VARM_UINT,
    PIO_MSG_PUT_VAR1_UINT,
    PIO_MSG_PUT_VAR1_INT,
    PIO_MSG_GET_VARA_FLOAT,
    PIO_MSG_GET_VARM_TEXT,
    PIO_MSG_PUT_VARS_FLOAT,
    PIO_MSG_GET_VAR1_TEXT,
    PIO_MSG_PUT_VARA_SHORT,
    PIO_MSG_PUT_VAR1_SCHAR,
    PIO_MSG_PUT_VARA_ULONGLONG,
    PIO_MSG_PUT_VARM_DOUBLE,
    PIO_MSG_GET_VARM_INT,
    PIO_MSG_PUT_VARA,
    PIO_MSG_PUT_VARA_LONG,
    PIO_MSG_GET_VARM_UINT,
    PIO_MSG_GET_VARM,
    PIO_MSG_PUT_VAR1_DOUBLE,
    PIO_MSG_GET_VARS_DOUBLE,
    PIO_MSG_GET_VARA_LONGLONG,
    PIO_MSG_GET_VAR_ULONGLONG,
    PIO_MSG_PUT_VARM_SCHAR,
    PIO_MSG_GET_VARA_ULONGLONG,
    PIO_MSG_GET_VAR_SHORT,
    PIO_MSG_GET_VARM_FLOAT,
    PIO_MSG_PUT_VAR_TEXT,
    PIO_MSG_PUT_VARS_INT,
    PIO_MSG_GET_VAR1_LONG,
    PIO_MSG_GET_VARM_LONG,
    PIO_MSG_GET_VARM_USHORT,
    PIO_MSG_PUT_VAR1_SHORT,
    PIO_MSG_PUT_VARS_LONGLONG,
    PIO_MSG_GET_VARM_LONGLONG,
    PIO_MSG_GET_VARS_TEXT,
    PIO_MSG_PUT_VARA_DOUBLE,
    PIO_MSG_PUT_VARS,
    PIO_MSG_PUT_VAR_UCHAR,
    PIO_MSG_GET_VAR1_UCHAR,
    PIO_MSG_PUT_VAR_LONG,
    PIO_MSG_GET_VARS,
    PIO_MSG_GET_VARM_SHORT,
    PIO_MSG_GET_VARM_ULONGLONG,
    PIO_MSG_PUT_VARM_LONGLONG,
    PIO_MSG_GET_VAR_SCHAR,
    PIO_MSG_GET_ATT_UBYTE,
    PIO_MSG_PUT_ATT_STRING,
    PIO_MSG_GET_ATT_STRING,
    PIO_MSG_PUT_ATT_UBYTE,
    PIO_MSG_INQ_VAR_FILL,
    PIO_MSG_DEF_VAR_FILL,
    PIO_MSG_DEF_VAR_DEFLATE,
    PIO_MSG_INQ_VAR_DEFLATE,
    PIO_MSG_INQ_VAR_SZIP,
    PIO_MSG_DEF_VAR_SZIP,
    PIO_MSG_DEF_VAR_FLETCHER32,
    PIO_MSG_INQ_VAR_FLETCHER32,
    PIO_MSG_DEF_VAR_CHUNKING,
    PIO_MSG_INQ_VAR_CHUNKING,
    PIO_MSG_DEF_VAR_ENDIAN,
    PIO_MSG_INQ_VAR_ENDIAN,
    PIO_MSG_SET_CHUNK_CACHE,
    PIO_MSG_GET_CHUNK_CACHE,
    PIO_MSG_SET_VAR_CHUNK_CACHE,
    PIO_MSG_GET_VAR_CHUNK_CACHE,
    PIO_MSG_INITDECOMP_DOF,
    PIO_MSG_WRITEDARRAY,
    PIO_MSG_WRITEDARRAYMULTI,
    PIO_MSG_SETFRAME,
    PIO_MSG_ADVANCEFRAME,
    PIO_MSG_READDARRAY,
    PIO_MSG_SETERRORHANDLING,
    PIO_MSG_SETLOGLEVEL,
    PIO_MSG_FREEDECOMP,
    PIO_MSG_CLOSE_FILE,
    PIO_MSG_DELETE_FILE,
    PIO_MSG_EXIT,
    PIO_MSG_GET_ATT,
    PIO_MSG_PUT_ATT,
    PIO_MSG_INQ_TYPE,
    PIO_MSG_INQ_UNLIMDIMS,
#ifdef NC_HAS_BZ2
    PIO_MSG_INQ_VAR_BZIP2,
    PIO_MSG_DEF_VAR_BZIP2,
#endif
#ifdef NC_HAS_ZSTD
    PIO_MSG_INQ_VAR_ZSTANDARD,
    PIO_MSG_DEF_VAR_ZSTANDARD,
#endif
    PIO_MSG_DEF_VAR_FILTER,
    PIO_MSG_INQ_VAR_FILTER_IDS,
    PIO_MSG_INQ_VAR_FILTER_INFO,
    PIO_MSG_INQ_FILTER_AVAIL,

#ifdef NC_HAS_QUANTIZE
    PIO_MSG_DEF_VAR_QUANTIZE,
    PIO_MSG_INQ_VAR_QUANTIZE,
#endif
};

#endif /* __PIO_INTERNAL__ */
