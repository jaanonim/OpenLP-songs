# -*- coding: utf-8 -*-
# vim: autoindent shiftwidth=4 expandtab textwidth=120 tabstop=4 softtabstop=4

###############################################################################
# OpenLP - Open Source Lyrics Projection                                      #
# --------------------------------------------------------------------------- #
# Copyright (c) 2008-2017 OpenLP Developers                                   #
# --------------------------------------------------------------------------- #
# This program is free software; you can redistribute it and/or modify it     #
# under the terms of the GNU General Public License as published by the Free  #
# Software Foundation; version 2 of the License.                              #
#                                                                             #
# This program is distributed in the hope that it will be useful, but WITHOUT #
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       #
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for    #
# more details.                                                               #
#                                                                             #
# You should have received a copy of the GNU General Public License along     #
# with this program; if not, write to the Free Software Foundation, Inc., 59  #
# Temple Place, Suite 330, Boston, MA 02111-1307 USA                          #
###############################################################################
"""
The :mod:`upgrade` module provides a way for the database and schema that is the
backend for the Songs plugin
"""
import logging

from sqlalchemy import Table, Column, ForeignKey, types
from sqlalchemy.sql.expression import func, false, null, text, and_, select

from openlp.core.lib.db import get_upgrade_op
from openlp.core.utils.db import drop_columns

log = logging.getLogger(__name__)
__version__ = 5


# TODO: When removing an upgrade path the ftw-data needs updating to the minimum supported version
def upgrade_1(session, metadata):
    """
    Version 1 upgrade.

    This upgrade removes the many-to-many relationship between songs and
    media_files and replaces it with a one-to-many, which is far more
    representative of the real relationship between the two entities.

    In order to facilitate this one-to-many relationship, a song_id column is
    added to the media_files table, and a weight column so that the media
    files can be ordered.

    :param session:
    :param metadata:
    """
    op = get_upgrade_op(session)
    songs_table = Table('songs', metadata, autoload=True)
    if 'media_files_songs' in [t.name for t in metadata.tables.values()]:
        op.drop_table('media_files_songs')
        op.add_column('media_files', Column('song_id', types.Integer(), server_default=null()))
        op.add_column('media_files', Column('weight', types.Integer(), server_default=text('0')))
        if metadata.bind.url.get_dialect().name != 'sqlite':
            # SQLite doesn't support ALTER TABLE ADD CONSTRAINT
            op.create_foreign_key('fk_media_files_song_id', 'media_files', 'songs', ['song_id', 'id'])
    else:
        log.warning('Skipping upgrade_1 step of upgrading the song db')


def upgrade_2(session, metadata):
    """
    Version 2 upgrade.

    This upgrade adds a create_date and last_modified date to the songs table
    """
    op = get_upgrade_op(session)
    songs_table = Table('songs', metadata, autoload=True)
    if 'create_date' not in [col.name for col in songs_table.c.values()]:
        op.add_column('songs', Column('create_date', types.DateTime(), default=func.now()))
        op.add_column('songs', Column('last_modified', types.DateTime(), default=func.now()))
    else:
        log.warning('Skipping upgrade_2 step of upgrading the song db')


def upgrade_3(session, metadata):
    """
    Version 3 upgrade.

    This upgrade adds a temporary song flag to the songs table
    """
    op = get_upgrade_op(session)
    songs_table = Table('songs', metadata, autoload=True)
    if 'temporary' not in [col.name for col in songs_table.c.values()]:
        if metadata.bind.url.get_dialect().name == 'sqlite':
            op.add_column('songs', Column('temporary', types.Boolean(create_constraint=False), server_default=false()))
        else:
            op.add_column('songs', Column('temporary', types.Boolean(), server_default=false()))
    else:
        log.warning('Skipping upgrade_3 step of upgrading the song db')


def upgrade_4(session, metadata):
    """
    Version 4 upgrade.

    This upgrade adds a column for author type to the authors_songs table
    """
    # Due to an incorrect check, this step was always skipped. Moved this code into upgrade 6.
    op = get_upgrade_op(session)
    authors_songs = Table('authors_songs', metadata, autoload=True)
    if 'author_type' not in [col.name for col in authors_songs.c.values()]:
        authors_songs_tmp = op.create_table(
            'authors_songs_tmp',
            Column('author_id', types.Integer(), ForeignKey('authors.id'), primary_key=True),
            Column('song_id', types.Integer(), ForeignKey('songs.id'), primary_key=True),
            Column('author_type', types.Unicode(255), primary_key=True,
                   nullable=False, server_default=text('""'))
        )
        op.execute('INSERT INTO authors_songs_tmp SELECT author_id, song_id, "" FROM authors_songs')
        op.drop_table('authors_songs')
        op.rename_table('authors_songs_tmp', 'authors_songs')
    else:
        log.warning('Skipping upgrade_4 step of upgrading the song db')


def upgrade_5(session, metadata):
    """
    Version 5 upgrade.

    This upgrade adds support for multiple songbooks
    """
    op = get_upgrade_op(session)
    songs_table = Table('songs', metadata, autoload=True)
    if 'song_book_id' not in [col.name for col in songs_table.c.values()]:
        log.warning('Skipping upgrade_5 step of upgrading the song db')
        return

    # Create the mapping table (songs <-> songbooks)
    songs_songbooks_table = op.create_table(
        'songs_songbooks',
        Column('songbook_id', types.Integer(), ForeignKey('song_books.id'), primary_key=True),
        Column('song_id', types.Integer(), ForeignKey('songs.id'), primary_key=True),
        Column('entry', types.Unicode(255), primary_key=True, nullable=False)
    )

    # Migrate old data
    op.execute('INSERT INTO songs_songbooks SELECT song_book_id, id, song_number FROM songs\
                WHERE song_book_id IS NOT NULL AND song_book_id <> 0 AND song_number IS NOT NULL')

    # Drop old columns
    if metadata.bind.url.get_dialect().name == 'sqlite':
        drop_columns(op, 'songs', ['song_book_id', 'song_number'])
    else:
        op.drop_constraint('songs_ibfk_1', 'songs', 'foreignkey')
        op.drop_column('songs', 'song_book_id')
        op.drop_column('songs', 'song_number')
