from cantools.database.can.c_source import generate
from cantools.database import load_file
from re import sub
import os

def purge_timestamps_from_generated_code(code: str) -> str:
    """
    Purges timestamps from the generated C code so that we can diff it in CI to
    check that the generated C code is up-to-date with the .dbc file
    """
    # Ex. replace:
    # "This file was generated by cantools version 32.4.0 Wed Jan  9 21:29:43 2019."
    # with:
    # "This file was generated by cantools"
    return sub(
        r'(This\sfile\swas\sgenerated\sby\scantools)\sversion\s\d*\.\d*\.\d*\s.*',
        r'\1', code)

def change_frame_id_capitalization(code: str) -> str:
    """
    Sets the symbol name in the FRAME_ID constant to lowercase for all CAN
    messages. This is done to allow us to reference both the constants and
    the associated function in C macros.
    """
    # Ex. replace:
    # "CANMSGS_SYMBOL1_FRAME_ID"
    # with:
    # "CANMSGS_symbol1_FRAME_ID"
    return sub(
        r'CANMSGS_(.*)_FRAME_ID',
        lambda match: r'CANMSGS_{}_FRAME_ID'.format(match.group(1).lower()),
        code)

def remove_app_prefix_from_structs(code: str) -> str:
    """
    Remove the App_ prefix from structs. This is done because only function
    names in the APP layer should have the App_ prefix, whereas struct names
    should not.
    """
    return sub(
        r'struct App_',
        r'struct ',
        code)

def remove_app_prefix_from_macros(code: str) -> str:
    """
    Remove the App_ prefix from structs. This is done because only function
    names in the APP layer should have the App_ prefix, whereas macro names
    should not.
    """
    return sub(
        r'APP_',
        r'',
        code)


def generate_cantools_c_code(database, database_name, source_path, header_path):
    """
    Generates C source code for the given .dbc file using cantools
    """
    # Prepare names for generate files
    filename_h = os.path.basename(header_path)
    filename_c = os.path.basename(source_path)

    # Generate C source code from cantools
    header, source, _, _ = generate(
        database,
        database_name,
        filename_h,
        filename_c,
        "",
        floating_point_numbers=True,
        # Updating values in bit-fields is NOT atomic, so we avoid using it.
        bit_fields=False
    )
    # Remove timestamps from generated source code because
    # the timestamps would pollute git diff
    header = purge_timestamps_from_generated_code(header)
    source = purge_timestamps_from_generated_code(source)

    # Remove unwanted prefix from struct and macro names
    header = remove_app_prefix_from_structs(header)
    header = remove_app_prefix_from_macros(header)
    source = remove_app_prefix_from_structs(source)
    source = remove_app_prefix_from_macros(source)

    # Generate output folders if they don't exist already
    source_dir = os.path.dirname(source_path)
    if not os.path.exists(source_dir):
        os.makedirs(source_dir)
    header_dir = os.path.dirname(header_path)
    if not os.path.exists(header_dir):
        os.mkdir(header_dir)

    # Save generated code to disk
    with open(os.path.join(header_dir, filename_h), 'w') as fout:
        fout.write(header)

    with open(os.path.join(source_dir, filename_c), 'w') as fout:
        fout.write(source)