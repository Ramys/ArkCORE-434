-- not forget: set you own password!!!!!!
-- Updated for MySQL 8.0 compatibility with caching_sha2_password authentication

CREATE USER 'arkania'@'localhost' IDENTIFIED BY 'SetYouOwnPasswordHere';
CREATE USER 'arkania'@'127.0.0.1' IDENTIFIED BY 'SetYouOwnPasswordHere';
CREATE USER 'arkania'@'%' IDENTIFIED BY 'SetYouOwnPasswordHere';

-- Set resource limits for the users
ALTER USER 'arkania'@'localhost' WITH MAX_QUERIES_PER_HOUR 0 MAX_CONNECTIONS_PER_HOUR 0 MAX_UPDATES_PER_HOUR 0;
ALTER USER 'arkania'@'127.0.0.1' WITH MAX_QUERIES_PER_HOUR 0 MAX_CONNECTIONS_PER_HOUR 0 MAX_UPDATES_PER_HOUR 0;
ALTER USER 'arkania'@'%' WITH MAX_QUERIES_PER_HOUR 0 MAX_CONNECTIONS_PER_HOUR 0 MAX_UPDATES_PER_HOUR 0;
